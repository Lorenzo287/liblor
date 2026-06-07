// SPDX-License-Identifier: MIT

#include "lor/map.h"

#if defined(LOR_LEAKCHECK)
#define LOR_MEMORY_NO_STDLIB_MACROS
#include "lor/memory.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
typedef max_align_t LorMapAlignment;
#else
typedef union LorMapAlignment {
    void *pointer;
    long double long_double;
    long long long_long;
} LorMapAlignment;
#endif

typedef union LorMapHeader {
    struct {
        size_t size;
        size_t capacity;
        size_t entry_size;
        size_t key_size;
        size_t bucket_capacity;
        size_t *buckets;
        LorMapConfig config;
    } values;
    LorMapAlignment alignment;
} LorMapHeader;

static LorMapHeader *lor_map__header(void *map) {
    return (LorMapHeader *)map - 1;
}

static const LorMapHeader *lor_map__header_const(const void *map) {
    return (const LorMapHeader *)map - 1;
}

static void *lor_map__read_ref(const void *map_ref) {
    void *map = NULL;
    if (map_ref != NULL) memcpy(&map, map_ref, sizeof(map));
    return map;
}

static void lor_map__write_ref(void *map_ref, void *map) {
    memcpy(map_ref, &map, sizeof(map));
}

static void *lor_map__malloc(size_t size) {
#if defined(LOR_LEAKCHECK)
    return lor_malloc_debug(size, __FILE__, __LINE__);
#else
    return malloc(size);
#endif
}

static void *lor_map__calloc(size_t count, size_t size) {
#if defined(LOR_LEAKCHECK)
    return lor_calloc_debug(count, size, __FILE__, __LINE__);
#else
    return calloc(count, size);
#endif
}

static void *lor_map__realloc(void *ptr, size_t size) {
#if defined(LOR_LEAKCHECK)
    return lor_realloc_debug(ptr, size, __FILE__, __LINE__);
#else
    return realloc(ptr, size);
#endif
}

static void lor_map__free(void *ptr) {
#if defined(LOR_LEAKCHECK)
    lor_free_debug(ptr, __FILE__, __LINE__);
#else
    free(ptr);
#endif
}

static uint64_t lor_map__hash_bytes(const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t lor_map__default_hash(const void *key, size_t key_size,
                                      void *context) {
    (void)context;
    return lor_map__hash_bytes(key, key_size);
}

static int lor_map__default_equal(const void *a, const void *b,
                                  size_t key_size, void *context) {
    (void)context;
    return memcmp(a, b, key_size) == 0;
}

static uint64_t lor_map__sv_hash(const void *key, size_t key_size,
                                 void *context) {
    (void)context;
    if (key_size != sizeof(LorStringView)) return 0;

    const LorStringView *view = (const LorStringView *)key;
    if (!lor_sv_is_valid(*view)) return 0;
    return lor_map__hash_bytes(view->data, view->size);
}

static int lor_map__sv_equal(const void *a, const void *b, size_t key_size,
                             void *context) {
    (void)context;
    if (key_size != sizeof(LorStringView)) return 0;
    return lor_sv_equal(*(const LorStringView *)a,
                        *(const LorStringView *)b);
}

static int lor_map__sv_valid(const void *key, size_t key_size, void *context) {
    (void)context;
    return key_size == sizeof(LorStringView) &&
           lor_sv_is_valid(*(const LorStringView *)key);
}

static LorStatus lor_map__sv_clone(void *destination, const void *source,
                                   size_t key_size, void *context) {
    (void)context;
    if (key_size != sizeof(LorStringView))
        return LOR_STATUS_INVALID_ARGUMENT;

    LorStringView view = *(const LorStringView *)source;
    if (!lor_sv_is_valid(view)) return LOR_STATUS_INVALID_ARGUMENT;

    LorStringView copy = LOR_STRING_VIEW_INIT;
    if (view.size != 0) {
        char *data = (char *)lor_map__malloc(view.size);
        if (data == NULL) return LOR_STATUS_OUT_OF_MEMORY;
        memcpy(data, view.data, view.size);
        copy = lor_sv_from_parts(data, view.size);
    }

    *(LorStringView *)destination = copy;
    return LOR_STATUS_OK;
}

static void lor_map__sv_drop(void *key, size_t key_size, void *context) {
    (void)context;
    if (key_size != sizeof(LorStringView)) return;

    LorStringView *view = (LorStringView *)key;
    lor_map__free((void *)view->data);
    *view = (LorStringView)LOR_STRING_VIEW_INIT;
}

LorMapConfig lor_map_config_bytes(void) {
    return (LorMapConfig){0};
}

LorMapConfig lor_map_config_string_view(LorMapKeyOwnership ownership) {
    LorMapConfig config = {
        .hash = lor_map__sv_hash,
        .equal = lor_map__sv_equal,
        .key_valid = lor_map__sv_valid,
    };
    if (ownership == LOR_MAP_KEY_OWNED) {
        config.clone = lor_map__sv_clone;
        config.drop = lor_map__sv_drop;
    } else if (ownership != LOR_MAP_KEY_BORROWED) {
        config.equal = NULL;
    }
    return config;
}

LorMapConfig lor_map_get_config(const void *map) {
    return map != NULL ? lor_map__header_const(map)->values.config
                       : lor_map_config_bytes();
}

int lor_map_config_equal(LorMapConfig a, LorMapConfig b) {
    return a.hash == b.hash && a.equal == b.equal &&
           a.key_valid == b.key_valid && a.clone == b.clone &&
           a.drop == b.drop && a.context == b.context;
}

static int lor_map__config_valid(LorMapConfig config) {
    return (config.hash == NULL) == (config.equal == NULL) &&
           (config.clone == NULL) == (config.drop == NULL);
}

static int lor_map__layout_valid(size_t entry_size, size_t key_size) {
    return key_size != 0 && entry_size >= key_size;
}

static int lor_map__matches_layout(const void *map, size_t entry_size,
                                   size_t key_size) {
    if (!lor_map__layout_valid(entry_size, key_size)) return 0;
    if (map == NULL) return 1;

    const LorMapHeader *header = lor_map__header_const(map);
    return header->values.entry_size == entry_size &&
           header->values.key_size == key_size;
}

static uint64_t lor_map__hash(const LorMapHeader *header, const void *key) {
    LorMapHashFn hash = header->values.config.hash;
    if (hash == NULL) hash = lor_map__default_hash;
    return hash(key, header->values.key_size,
                header->values.config.context);
}

static int lor_map__equal(const LorMapHeader *header, const void *a,
                          const void *b) {
    LorMapEqualFn equal = header->values.config.equal;
    if (equal == NULL) equal = lor_map__default_equal;
    return equal(a, b, header->values.key_size,
                 header->values.config.context);
}

static int lor_map__key_valid(const LorMapHeader *header, const void *key) {
    LorMapKeyValidFn key_valid = header->values.config.key_valid;
    return key_valid == NULL ||
           key_valid(key, header->values.key_size,
                     header->values.config.context);
}

static void *lor_map__entry(void *map, size_t index) {
    LorMapHeader *header = lor_map__header(map);
    return (unsigned char *)map + index * header->values.entry_size;
}

static const void *lor_map__entry_const(const void *map, size_t index) {
    const LorMapHeader *header = lor_map__header_const(map);
    return (const unsigned char *)map +
           index * header->values.entry_size;
}

static size_t lor_map__bucket_for(const LorMapHeader *header, uint64_t hash) {
    return (size_t)hash & (header->values.bucket_capacity - 1u);
}

static size_t lor_map__find_bucket(const void *map, const void *key) {
    const LorMapHeader *header = lor_map__header_const(map);
    if (header->values.bucket_capacity == 0) return SIZE_MAX;

    size_t mask = header->values.bucket_capacity - 1u;
    size_t bucket = lor_map__bucket_for(header, lor_map__hash(header, key));
    for (;;) {
        size_t stored = header->values.buckets[bucket];
        if (stored == 0) return SIZE_MAX;

        size_t index = stored - 1u;
        if (lor_map__equal(header, lor_map__entry_const(map, index), key))
            return bucket;
        bucket = (bucket + 1u) & mask;
    }
}

static void lor_map__insert_bucket(void *map, size_t index) {
    LorMapHeader *header = lor_map__header(map);
    const void *key = lor_map__entry_const(map, index);
    size_t mask = header->values.bucket_capacity - 1u;
    size_t bucket = lor_map__bucket_for(header, lor_map__hash(header, key));
    while (header->values.buckets[bucket] != 0)
        bucket = (bucket + 1u) & mask;
    header->values.buckets[bucket] = index + 1u;
}

static void lor_map__fill_buckets(void *map, size_t *buckets,
                                  size_t bucket_capacity) {
    LorMapHeader *header = lor_map__header(map);
    header->values.buckets = buckets;
    header->values.bucket_capacity = bucket_capacity;

    for (size_t i = 0; i < header->values.size; ++i)
        lor_map__insert_bucket(map, i);
}

static LorStatus lor_map__entry_capacity(size_t current, size_t required,
                                         size_t *capacity) {
    size_t result = current < 8u ? 8u : current;
    while (result < required) {
        if (result > SIZE_MAX / 2u) {
            result = required;
            break;
        }
        result *= 2u;
    }
    *capacity = result;
    return LOR_STATUS_OK;
}

static LorStatus lor_map__bucket_capacity(size_t required_entries,
                                          size_t *capacity) {
    if (required_entries == 0) {
        *capacity = 0;
        return LOR_STATUS_OK;
    }
    if (required_entries > SIZE_MAX / 4u)
        return LOR_STATUS_OVERFLOW;

    size_t required = required_entries + required_entries / 3u + 1u;
    size_t result = 8u;
    while (result < required) {
        if (result > SIZE_MAX / 2u) return LOR_STATUS_OVERFLOW;
        result *= 2u;
    }
    *capacity = result;
    return LOR_STATUS_OK;
}

static LorStatus lor_map__allocate_header(size_t entry_size, size_t key_size,
                                          LorMapConfig config,
                                          LorMapHeader **header) {
    LorMapHeader *result =
        (LorMapHeader *)lor_map__malloc(sizeof(LorMapHeader));
    if (result == NULL) return LOR_STATUS_OUT_OF_MEMORY;

    memset(result, 0, sizeof(*result));
    result->values.entry_size = entry_size;
    result->values.key_size = key_size;
    result->values.config = config;
    *header = result;
    return LOR_STATUS_OK;
}

LorStatus lor_map_init_raw(void *map_ref, size_t entry_size, size_t key_size,
                           LorMapConfig config) {
    if (map_ref == NULL || lor_map__read_ref(map_ref) != NULL ||
        !lor_map__layout_valid(entry_size, key_size) ||
        !lor_map__config_valid(config))
        return LOR_STATUS_INVALID_ARGUMENT;

    LorMapHeader *header = NULL;
    LorStatus status =
        lor_map__allocate_header(entry_size, key_size, config, &header);
    if (status != LOR_STATUS_OK) return status;

    lor_map__write_ref(map_ref, header + 1);
    return LOR_STATUS_OK;
}

size_t lor_map_size(const void *map) {
    return map != NULL ? lor_map__header_const(map)->values.size : 0;
}

size_t lor_map_capacity(const void *map) {
    return map != NULL ? lor_map__header_const(map)->values.capacity : 0;
}

size_t lor_map_entry_size(const void *map) {
    return map != NULL ? lor_map__header_const(map)->values.entry_size : 0;
}

size_t lor_map_key_size(const void *map) {
    return map != NULL ? lor_map__header_const(map)->values.key_size : 0;
}

LorStatus lor_map_reserve_raw(void *map_ref, size_t entry_size,
                              size_t key_size, size_t capacity) {
    if (map_ref == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    void *map = lor_map__read_ref(map_ref);
    if (!lor_map__matches_layout(map, entry_size, key_size))
        return LOR_STATUS_INVALID_ARGUMENT;
    if (capacity <= lor_map_capacity(map)) return LOR_STATUS_OK;

    size_t new_capacity = 0;
    LorStatus status = lor_map__entry_capacity(
        lor_map_capacity(map), capacity, &new_capacity);
    if (status != LOR_STATUS_OK) return status;
    if (new_capacity >
        (SIZE_MAX - sizeof(LorMapHeader)) / entry_size)
        return LOR_STATUS_OVERFLOW;

    size_t new_bucket_capacity = 0;
    status = lor_map__bucket_capacity(new_capacity, &new_bucket_capacity);
    if (status != LOR_STATUS_OK) return status;
    if (new_bucket_capacity > SIZE_MAX / sizeof(size_t))
        return LOR_STATUS_OVERFLOW;

    size_t *new_buckets = (size_t *)lor_map__calloc(
        new_bucket_capacity, sizeof(*new_buckets));
    if (new_buckets == NULL) return LOR_STATUS_OUT_OF_MEMORY;

    LorMapHeader *header = map != NULL ? lor_map__header(map) : NULL;
    if (header == NULL) {
        status = lor_map__allocate_header(
            entry_size, key_size, lor_map_config_bytes(), &header);
        if (status != LOR_STATUS_OK) {
            lor_map__free(new_buckets);
            return status;
        }
    }

    size_t allocation_size =
        sizeof(LorMapHeader) + new_capacity * entry_size;
    LorMapHeader *new_header =
        (LorMapHeader *)lor_map__realloc(header, allocation_size);
    if (new_header == NULL) {
        if (map == NULL) lor_map__free(header);
        lor_map__free(new_buckets);
        return LOR_STATUS_OUT_OF_MEMORY;
    }

    void *new_map = new_header + 1;
    size_t *old_buckets = new_header->values.buckets;
    new_header->values.capacity = new_capacity;
    lor_map__fill_buckets(new_map, new_buckets, new_bucket_capacity);
    lor_map__free(old_buckets);
    lor_map__write_ref(map_ref, new_map);
    return LOR_STATUS_OK;
}

static int lor_map__entry_alias(const void *map, const void *entry) {
    if (map == NULL || entry == NULL) return 0;

    const LorMapHeader *header = lor_map__header_const(map);
    if (header->values.size > SIZE_MAX / header->values.entry_size)
        return -1;

    size_t initialized_bytes =
        header->values.size * header->values.entry_size;
    uintptr_t base = (uintptr_t)map;
    uintptr_t source = (uintptr_t)entry;
    if (initialized_bytes > UINTPTR_MAX - base) return 0;
    if (source < base || source > base + initialized_bytes) return 0;

    size_t offset = (size_t)(source - base);
    if (offset > initialized_bytes ||
        header->values.entry_size > initialized_bytes - offset)
        return -1;
    return 1;
}

static LorStatus lor_map__prepare_entry(const LorMapHeader *header,
                                        const void *entry, int alias,
                                        void **prepared) {
    LorMapKeyCloneFn clone = header->values.config.clone;
    if (clone == NULL && alias == 0) {
        *prepared = NULL;
        return LOR_STATUS_OK;
    }

    void *copy = lor_map__malloc(header->values.entry_size);
    if (copy == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    memcpy(copy, entry, header->values.entry_size);

    if (clone != NULL) {
        memset(copy, 0, header->values.key_size);
        LorStatus status =
            clone(copy, entry, header->values.key_size,
                  header->values.config.context);
        if (status != LOR_STATUS_OK) {
            lor_map__free(copy);
            return status;
        }
    }

    *prepared = copy;
    return LOR_STATUS_OK;
}

static void lor_map__drop_prepared(const LorMapHeader *header, void *entry,
                                   int owns_key) {
    if (entry == NULL) return;
    if (owns_key)
        header->values.config.drop(entry, header->values.key_size,
                                   header->values.config.context);
    lor_map__free(entry);
}

LorStatus lor_map_set_raw(void *map_ref, size_t entry_size, size_t key_size,
                          const void *entry) {
    if (map_ref == NULL || entry == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;

    void *map = lor_map__read_ref(map_ref);
    if (!lor_map__matches_layout(map, entry_size, key_size))
        return LOR_STATUS_INVALID_ARGUMENT;

    int created = map == NULL;
    if (created) {
        LorStatus status =
            lor_map_init_raw(map_ref, entry_size, key_size,
                             lor_map_config_bytes());
        if (status != LOR_STATUS_OK) return status;
        map = lor_map__read_ref(map_ref);
    }

    LorMapHeader *header = lor_map__header(map);
    if (!lor_map__key_valid(header, entry))
        return LOR_STATUS_INVALID_ARGUMENT;
    size_t bucket = lor_map__find_bucket(map, entry);
    int alias = lor_map__entry_alias(map, entry);
    if (alias < 0) return LOR_STATUS_INVALID_ARGUMENT;

    void *prepared = NULL;
    LorStatus status =
        lor_map__prepare_entry(header, entry, alias, &prepared);
    if (status != LOR_STATUS_OK) return status;
    const void *source = prepared != NULL ? prepared : entry;
    int prepared_owns_key = header->values.config.clone != NULL;

    if (bucket != SIZE_MAX) {
        size_t index = header->values.buckets[bucket] - 1u;
        void *destination = lor_map__entry(map, index);
        if (prepared_owns_key)
            header->values.config.drop(
                destination, header->values.key_size,
                header->values.config.context);
        memmove(destination, source, header->values.entry_size);
        lor_map__free(prepared);
        return LOR_STATUS_OK;
    }

    size_t old_size = header->values.size;
    if (old_size == SIZE_MAX) {
        lor_map__drop_prepared(header, prepared, prepared_owns_key);
        if (created) lor_map_deinit(map_ref);
        return LOR_STATUS_OVERFLOW;
    }

    status = lor_map_reserve_raw(map_ref, entry_size, key_size, old_size + 1u);
    if (status != LOR_STATUS_OK) {
        lor_map__drop_prepared(header, prepared, prepared_owns_key);
        if (created) lor_map_deinit(map_ref);
        return status;
    }

    map = lor_map__read_ref(map_ref);
    header = lor_map__header(map);
    void *destination = lor_map__entry(map, old_size);
    memcpy(destination, source, entry_size);
    header->values.size = old_size + 1u;
    lor_map__insert_bucket(map, old_size);
    lor_map__free(prepared);
    return LOR_STATUS_OK;
}

void *lor_map_find_raw(void *map, size_t entry_size, size_t key_size,
                       const void *key) {
    if (map == NULL || key == NULL ||
        !lor_map__matches_layout(map, entry_size, key_size))
        return NULL;

    if (!lor_map__key_valid(lor_map__header_const(map), key)) return NULL;
    size_t bucket = lor_map__find_bucket(map, key);
    if (bucket == SIZE_MAX) return NULL;
    return lor_map__entry(
        map, lor_map__header(map)->values.buckets[bucket] - 1u);
}

const void *lor_map_find_const_raw(const void *map, size_t entry_size,
                                   size_t key_size, const void *key) {
    return lor_map_find_raw((void *)map, entry_size, key_size, key);
}

int lor_map_contains_raw(const void *map, size_t entry_size, size_t key_size,
                         const void *key) {
    return lor_map_find_const_raw(map, entry_size, key_size, key) != NULL;
}

static size_t lor_map__probe_distance(size_t ideal, size_t actual,
                                      size_t mask) {
    return (actual - ideal) & mask;
}

static void lor_map__erase_bucket(void *map, size_t bucket) {
    LorMapHeader *header = lor_map__header(map);
    size_t mask = header->values.bucket_capacity - 1u;
    size_t hole = bucket;
    size_t next = (hole + 1u) & mask;

    while (header->values.buckets[next] != 0) {
        size_t index = header->values.buckets[next] - 1u;
        const void *key = lor_map__entry_const(map, index);
        size_t ideal = lor_map__bucket_for(header, lor_map__hash(header, key));
        if (lor_map__probe_distance(ideal, next, mask) >
            lor_map__probe_distance(ideal, hole, mask)) {
            header->values.buckets[hole] =
                header->values.buckets[next];
            hole = next;
        }
        next = (next + 1u) & mask;
    }
    header->values.buckets[hole] = 0;
}

int lor_map_remove_raw(void *map, size_t entry_size, size_t key_size,
                       const void *key) {
    if (map == NULL || key == NULL ||
        !lor_map__matches_layout(map, entry_size, key_size))
        return 0;

    LorMapHeader *header = lor_map__header(map);
    if (!lor_map__key_valid(header, key)) return 0;
    size_t bucket = lor_map__find_bucket(map, key);
    if (bucket == SIZE_MAX) return 0;

    size_t index = header->values.buckets[bucket] - 1u;
    size_t last = header->values.size - 1u;
    lor_map__erase_bucket(map, bucket);

    void *removed = lor_map__entry(map, index);
    if (header->values.config.drop != NULL)
        header->values.config.drop(
            removed, header->values.key_size,
            header->values.config.context);

    if (index != last) {
        void *last_entry = lor_map__entry(map, last);
        memcpy(removed, last_entry, header->values.entry_size);

        size_t moved_bucket = lor_map__find_bucket(map, removed);
        if (moved_bucket != SIZE_MAX)
            header->values.buckets[moved_bucket] = index + 1u;
    }

    header->values.size = last;
    return 1;
}

void lor_map_clear(void *map) {
    if (map == NULL) return;

    LorMapHeader *header = lor_map__header(map);
    if (header->values.config.drop != NULL) {
        for (size_t i = 0; i < header->values.size; ++i)
            header->values.config.drop(
                lor_map__entry(map, i), header->values.key_size,
                header->values.config.context);
    }
    header->values.size = 0;
    if (header->values.buckets != NULL)
        memset(header->values.buckets, 0,
               header->values.bucket_capacity * sizeof(size_t));
}

void lor_map_deinit(void *map_ref) {
    if (map_ref == NULL) return;

    void *map = lor_map__read_ref(map_ref);
    if (map != NULL) {
        LorMapHeader *header = lor_map__header(map);
        lor_map_clear(map);
        lor_map__free(header->values.buckets);
        lor_map__free(header);
    }
    lor_map__write_ref(map_ref, NULL);
}
