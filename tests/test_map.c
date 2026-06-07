// SPDX-License-Identifier: MIT

#include "lor/map.h"

#include "lor/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef LOR_MAP_ENTRY(int, int) IntEntry;
typedef LOR_MAP_ENTRY(LorStringView, int) WordEntry;

typedef struct PairKey {
    int x;
    int y;
} PairKey;

typedef struct PairEntry {
    PairKey key;
    int value;
} PairEntry;

static uint64_t constant_hash(const void *key, size_t key_size,
                              void *context) {
    (void)key;
    (void)key_size;
    (void)context;
    return UINT64_MAX;
}

static int int_equal(const void *a, const void *b, size_t key_size,
                     void *context) {
    (void)context;
    return key_size == sizeof(int) &&
           *(const int *)a == *(const int *)b;
}

static uint64_t pair_hash(const void *key, size_t key_size, void *context) {
    (void)context;
    if (key_size != sizeof(PairKey)) return 0;
    const PairKey *pair = (const PairKey *)key;
    return (uint64_t)(uint32_t)pair->x * UINT64_C(0x9e3779b1) ^
           (uint64_t)(uint32_t)pair->y;
}

static int pair_equal(const void *a, const void *b, size_t key_size,
                      void *context) {
    (void)context;
    if (key_size != sizeof(PairKey)) return 0;
    const PairKey *left = (const PairKey *)a;
    const PairKey *right = (const PairKey *)b;
    return left->x == right->x && left->y == right->y;
}

static int test_map_byte_keys(void) {
    IntEntry *map = LOR_MAP_INIT;
    CHECK(lor_map_size(map) == 0);
    CHECK(lor_map_capacity(map) == 0);

    IntEntry one = {.key = 1, .value = 10};
    CHECK(lor_map_set(map, one) == LOR_STATUS_OK);
    CHECK(lor_map_put_as(map, IntEntry, 2, 20) == LOR_STATUS_OK);
#if LOR_HAS_MAP_AUTO
    int key = 3;
    CHECK(lor_map_put_auto(map, key++, 30) == LOR_STATUS_OK);
    CHECK(key == 4);
#else
    CHECK(lor_map_put_as(map, IntEntry, 3, 30) == LOR_STATUS_OK);
#endif

    CHECK(lor_map_size(map) == 3);
    CHECK(lor_map_entry_size(map) == sizeof(*map));
    CHECK(lor_map_key_size(map) == sizeof(map->key));

    int two = 2;
    IntEntry *found = lor_map_find(map, two);
    CHECK(found != NULL && found->value == 20);
    CHECK(lor_map_contains(map, two));
    CHECK(!lor_map_contains_as(map, int, 99));

#if LOR_HAS_MAP_AUTO
    found = lor_map_find_auto(map, 3);
    CHECK(found != NULL && found->value == 30);
#endif

    CHECK(lor_map_put_as(map, IntEntry, 2, 200) == LOR_STATUS_OK);
    CHECK(lor_map_size(map) == 3);
    found = lor_map_find(map, two);
    CHECK(found != NULL && found->value == 200);

    int sum = 0;
    for (size_t i = 0; i < lor_map_size(map); ++i) sum += map[i].value;
    CHECK(sum == 240);

    CHECK(lor_map_reserve(map, 100) == LOR_STATUS_OK);
    CHECK(lor_map_capacity(map) >= 100);
    CHECK(lor_map_find(map, two) != NULL);

    CHECK(lor_map_remove(map, two));
    CHECK(!lor_map_remove(map, two));
    CHECK(lor_map_size(map) == 2);
    CHECK(lor_map_find_as(map, int, 1) != NULL);
    CHECK(lor_map_find_as(map, int, 3) != NULL);

    size_t capacity = lor_map_capacity(map);
    lor_map_clear(map);
    CHECK(lor_map_size(map) == 0);
    CHECK(lor_map_capacity(map) == capacity);
    CHECK(lor_map_put_as(map, IntEntry, 7, 70) == LOR_STATUS_OK);

    lor_map_deinit(&map);
    CHECK(map == NULL);
    lor_map_deinit(&map);
    lor_map_deinit(NULL);
    return 0;
}

static int test_map_collision_deletion(void) {
    IntEntry *map = LOR_MAP_INIT;
    LorMapConfig config = {
        .hash = constant_hash,
        .equal = int_equal,
    };
    CHECK(lor_map_init(map, config) == LOR_STATUS_OK);

    for (int i = 0; i < 200; ++i)
        CHECK(lor_map_put_as(map, IntEntry, i, i * 10) ==
              LOR_STATUS_OK);

    for (int i = 1; i < 200; i += 2)
        CHECK(lor_map_remove(map, i));

    CHECK(lor_map_size(map) == 100);
    for (int i = 0; i < 200; ++i) {
        IntEntry *entry = lor_map_find(map, i);
        if ((i & 1) == 0)
            CHECK(entry != NULL && entry->value == i * 10);
        else
            CHECK(entry == NULL);
    }

    for (int i = 0; i < 200; i += 2)
        CHECK(lor_map_remove(map, i));
    CHECK(lor_map_size(map) == 0);

    lor_map_deinit(&map);
    return 0;
}

static int test_map_custom_struct_keys(void) {
    PairEntry *map = LOR_MAP_INIT;
    LorMapConfig config = {
        .hash = pair_hash,
        .equal = pair_equal,
    };
    CHECK(lor_map_init(map, config) == LOR_STATUS_OK);

    PairEntry entry = {.key = {.x = 2, .y = 3}, .value = 23};
    CHECK(lor_map_set(map, entry) == LOR_STATUS_OK);
    PairKey key = {.x = 2, .y = 3};
    PairEntry *found = lor_map_find(map, key);
    CHECK(found != NULL && found->value == 23);
    CHECK(lor_map_remove(map, key));

    lor_map_deinit(&map);
    return 0;
}

static int test_map_string_views(void) {
    WordEntry *borrowed = LOR_MAP_INIT;
    CHECK(lor_map_init(
              borrowed,
              lor_map_config_string_view(LOR_MAP_KEY_BORROWED)) ==
          LOR_STATUS_OK);
    CHECK(lor_map_put_as(borrowed, WordEntry,
                         lor_sv_from_cstr("alpha"), 1) ==
          LOR_STATUS_OK);
    LorStringView invalid = {.data = NULL, .size = 1};
    WordEntry invalid_entry = {.key = invalid, .value = 0};
    CHECK(lor_map_set(borrowed, invalid_entry) ==
          LOR_STATUS_INVALID_ARGUMENT);

    LorStringView alpha = lor_sv_from_cstr("alpha");
    WordEntry *found = lor_map_find(borrowed, alpha);
    CHECK(found != NULL && found->value == 1);
    lor_map_deinit(&borrowed);

    WordEntry *owned = LOR_MAP_INIT;
    CHECK(lor_map_init(
              owned, lor_map_config_string_view(LOR_MAP_KEY_OWNED)) ==
          LOR_STATUS_OK);

    char source[] = "mutable";
    LorStringView source_view = lor_sv_from_cstr(source);
    WordEntry first = {.key = source_view, .value = 10};
    CHECK(lor_map_set(owned, first) == LOR_STATUS_OK);
    memset(source, 'x', sizeof(source) - 1u);

    LorStringView mutable_key = lor_sv_from_cstr("mutable");
    found = lor_map_find(owned, mutable_key);
    CHECK(found != NULL && found->value == 10);
    CHECK(found->key.data != source);

    const char *old_key_data = found->key.data;
    CHECK(lor_map_put_as(owned, WordEntry,
                         lor_sv_from_cstr("mutable"), 20) ==
          LOR_STATUS_OK);
    found = lor_map_find(owned, mutable_key);
    CHECK(found != NULL && found->value == 20);
    CHECK(found->key.data != old_key_data);

    LorStringView empty = LOR_STRING_VIEW_INIT;
    WordEntry empty_entry = {.key = empty, .value = 5};
    CHECK(lor_map_set(owned, empty_entry) == LOR_STATUS_OK);
    found = lor_map_find(owned, empty);
    CHECK(found != NULL && found->value == 5);

    size_t size = lor_map_size(owned);
    CHECK(lor_map_set(owned, invalid_entry) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_map_size(owned) == size);

    CHECK(lor_map_remove(owned, mutable_key));
    CHECK(lor_map_find(owned, mutable_key) == NULL);
    lor_map_clear(owned);
    CHECK(lor_map_size(owned) == 0);
    lor_map_deinit(&owned);
    return 0;
}

static int test_map_failures(void) {
    IntEntry *map = LOR_MAP_INIT;
    LorMapConfig invalid = {.hash = constant_hash};
    CHECK(lor_map_init(map, invalid) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(map == NULL);

    CHECK(lor_map_init_raw(NULL, sizeof(IntEntry), sizeof(int),
                           lor_map_config_bytes()) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_map_init_raw(&map, 0, sizeof(int),
                           lor_map_config_bytes()) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_map_reserve(map, SIZE_MAX) == LOR_STATUS_OVERFLOW);
    CHECK(map == NULL);

    CHECK(lor_map_put_as(map, IntEntry, 1, 1) == LOR_STATUS_OK);
    IntEntry *original = map;
    size_t size = lor_map_size(map);
    CHECK(lor_map_reserve_raw(&map, sizeof(IntEntry), sizeof(short), 20) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(map == original);
    CHECK(lor_map_size(map) == size);

    CHECK(lor_map_find_raw(map, sizeof(IntEntry), sizeof(short), &size) ==
          NULL);
    lor_map_deinit(&map);
    return 0;
}

static int test_map_cleanup_and_leakcheck(void) {
#if defined(LOR_LEAKCHECK)
    size_t before = lor_leakcheck_count();
    {
        LOR_AUTO_MAP WordEntry *map = LOR_MAP_INIT;
        CHECK(lor_map_init(
                  map, lor_map_config_string_view(LOR_MAP_KEY_OWNED)) ==
              LOR_STATUS_OK);
        CHECK(lor_map_put_as(map, WordEntry,
                             lor_sv_from_cstr("tracked"), 1) ==
              LOR_STATUS_OK);
        CHECK(lor_leakcheck_count() > before);
    }
    CHECK(lor_leakcheck_count() == before);
#endif
    return 0;
}

int main(void) {
    CHECK(test_map_byte_keys() == 0);
    CHECK(test_map_collision_deletion() == 0);
    CHECK(test_map_custom_struct_keys() == 0);
    CHECK(test_map_string_views() == 0);
    CHECK(test_map_failures() == 0);
    CHECK(test_map_cleanup_and_leakcheck() == 0);

    puts("test_map: ok");
    return 0;
}
