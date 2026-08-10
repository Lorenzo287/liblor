// SPDX-License-Identifier: MIT

#ifndef LOR_MAP_H
#define LOR_MAP_H

#include <stddef.h>
#include <stdint.h>

#include "lor/features.h"
#include "lor/status.h"
#include "lor/string.h"  // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

/* Typed hash-map entry helper.

   Map entries must begin with a field named `key`. A field named `value` is
   required only by the `lor_map_put_*` convenience macros. */
#define LOR_MAP_ENTRY(key_type, value_type) \
    struct {                                \
        key_type key;                       \
        value_type value;                   \
    }

#define LOR_MAP_INIT NULL

typedef uint64_t (*LorMapHashFn)(const void *key, size_t key_size, void *context);
typedef int (*LorMapEqualFn)(const void *a, const void *b, size_t key_size,
                             void *context);
typedef int (*LorMapKeyValidFn)(const void *key, size_t key_size, void *context);

/* Copies one key into `destination`.

   Return `LOR_STATUS_OK` only when the destination key is ready to own. On
   failure, the map does not call `drop` for the incomplete destination. */
typedef LorStatus (*LorMapKeyCloneFn)(void *destination, const void *source,
                                      size_t key_size, void *context);

// Releases resources owned by one stored key.
typedef void (*LorMapKeyDropFn)(void *key, size_t key_size, void *context);

typedef struct LorMapConfig {
    LorMapHashFn hash;
    LorMapEqualFn equal;
    LorMapKeyValidFn key_valid;
    LorMapKeyCloneFn clone;
    LorMapKeyDropFn drop;
    void *context;
} LorMapConfig;

typedef enum {
    LOR_MAP_KEY_BORROWED = 0,
    LOR_MAP_KEY_OWNED
} LorMapKeyOwnership;

// Returns byte hashing/equality with shallow key copies.
LorMapConfig lor_map_config_bytes(void);

/* Returns content hashing/equality for `LorStringView` keys.

   Borrowed maps store the original views. Owned maps copy each key's bytes and
   release them on replacement, removal, clear, or deinit. */
LorMapConfig lor_map_config_string_view(LorMapKeyOwnership ownership);

// Returns the map's key configuration, or byte-key configuration for `NULL`.
LorMapConfig lor_map_get_config(const void *map);

// Returns non-zero when every callback and context pointer matches.
int lor_map_config_equal(LorMapConfig a, LorMapConfig b);

/* Initializes an empty map with custom key behavior.

   `entry_size` is `sizeof *map`; `key_size` is `sizeof map->key`. `map_ref`
   must point to a `NULL` handle. `hash` and `equal` must either both be set or
   both be `NULL`; the same rule applies to `clone` and `drop`. `key_valid` is
   optional and rejects invalid keys before hashing. */
LorStatus lor_map_init_raw(void *map_ref, size_t entry_size, size_t key_size,
                           LorMapConfig config);

// Returns the number of entries, or zero for `NULL`.
size_t lor_map_size(const void *map);

// Returns the allocated dense-entry capacity, or zero for `NULL`.
size_t lor_map_capacity(const void *map);

// Returns the stored entry size, or zero for `NULL`.
size_t lor_map_entry_size(const void *map);

// Returns the stored key size, or zero for `NULL`.
size_t lor_map_key_size(const void *map);

/* Ensures room for at least `capacity` entries.

   A `NULL` map is initialized with byte-key behavior. Existing entries and the
   original handle are unchanged on failure. */
LorStatus lor_map_reserve_raw(void *map_ref, size_t entry_size, size_t key_size,
                              size_t capacity);

/* Inserts or replaces an entry.

   The entry's first field is its key. A `NULL` map is initialized with
   byte-key behavior. The map is unchanged on failure. */
LorStatus lor_map_set_raw(void *map_ref, size_t entry_size, size_t key_size,
                          const void *entry);

/* Returns the mutable entry matching `key`, or `NULL`.

   The returned `void *` converts to the map's entry-pointer type in C. Stored
   keys must not be modified through the returned entry. */
void *lor_map_find_raw(void *map, size_t entry_size, size_t key_size,
                       const void *key);

// Const-qualified form of `lor_map_find_raw`.
const void *lor_map_find_const_raw(const void *map, size_t entry_size,
                                   size_t key_size, const void *key);

// Returns non-zero when `key` is present.
int lor_map_contains_raw(const void *map, size_t entry_size, size_t key_size,
                         const void *key);

// Removes `key` when present and returns non-zero.
int lor_map_remove_raw(void *map, size_t entry_size, size_t key_size,
                       const void *key);

// Removes all entries and owned keys while retaining map capacity and config.
void lor_map_clear(void *map);

// Releases entries, buckets, owned keys, and resets the handle to `NULL`.
void lor_map_deinit(void *map_ref);

/* Typed portable operations.

   `lor_map_set`, `find`, `contains`, and `remove` take lvalues of the exact
   entry or key type. In C, the `_as` forms construct C99 compound literals. */
#define lor_map_init(map, config) \
    lor_map_init_raw(&(map), sizeof *(map), sizeof(map)->key, (config))
#define lor_map_reserve(map, capacity) \
    lor_map_reserve_raw(&(map), sizeof *(map), sizeof(map)->key, (capacity))
#define lor_map_set(map, entry) \
    lor_map_set_raw(&(map), sizeof *(map), sizeof(map)->key, &(entry))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_map_set_as(map, type, ...) \
    lor_map_set_raw(&(map), sizeof *(map), sizeof(map)->key, &(type){__VA_ARGS__})
#define lor_map_put_as(map, type, key_value, value_value) \
    lor_map_set_as(map, type, .key = (key_value), .value = (value_value))
#endif
#define lor_map_find(map, key_value) \
    lor_map_find_raw((map), sizeof *(map), sizeof(map)->key, &(key_value))
#define lor_map_find_const(map, key_value) \
    lor_map_find_const_raw((map), sizeof *(map), sizeof(map)->key, &(key_value))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_map_find_as(map, type, ...) \
    lor_map_find_raw((map), sizeof *(map), sizeof(map)->key, &(type){__VA_ARGS__})
#endif
#define lor_map_contains(map, key_value) \
    lor_map_contains_raw((map), sizeof *(map), sizeof(map)->key, &(key_value))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_map_contains_as(map, type, ...)                      \
    lor_map_contains_raw((map), sizeof *(map), sizeof(map)->key, &(type){__VA_ARGS__})
#endif
#define lor_map_remove(map, key_value) \
    lor_map_remove_raw((map), sizeof *(map), sizeof(map)->key, &(key_value))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_map_remove_as(map, type, ...) \
    lor_map_remove_raw((map), sizeof *(map), sizeof(map)->key, &(type){__VA_ARGS__})
#endif

/* GCC/Clang/TCC convenience operations.

   These infer destination types, evaluate each supplied expression once, and
   perform normal assignment conversion into temporary key/value objects. */
#if LOR_HAS_TYPEOF && LOR_HAS_STATEMENT_EXPRESSIONS
#define LOR_HAS_MAP_AUTO 1
#define lor_map_put_auto(map, key_value, value_value)                                     \
    __extension__({                                                                       \
        lor_typeof(*(map)) lor_map__entry = {.key = (key_value), .value = (value_value)}; \
        lor_map_set_raw(&(map), sizeof *(map), sizeof(map)->key, &lor_map__entry);        \
    })
#define lor_map_find_auto(map, key_value)                                         \
    __extension__({                                                               \
        lor_typeof((map)->key) lor_map__key = (key_value);                        \
        (lor_typeof(map))lor_map_find_raw((map), sizeof *(map), sizeof(map)->key, \
                                          &lor_map__key);                         \
    })
#define lor_map_contains_auto(map, key_value)                                        \
    __extension__({                                                                  \
        lor_typeof((map)->key) lor_map__key = (key_value);                           \
        lor_map_contains_raw((map), sizeof *(map), sizeof(map)->key, &lor_map__key); \
    })
#define lor_map_remove_auto(map, key_value)                                        \
    __extension__({                                                                \
        lor_typeof((map)->key) lor_map__key = (key_value);                         \
        lor_map_remove_raw((map), sizeof *(map), sizeof(map)->key, &lor_map__key); \
    })
#else
#define LOR_HAS_MAP_AUTO 0
#endif

/* Scope-exit cleanup for maps on compilers with cleanup attributes.

   Unsupported compilers leave `LOR_AUTO_MAP` empty, so explicit
   `lor_map_deinit` remains required for portable ownership paths. */
#if LOR_HAS_CLEANUP_ATTRIBUTE
static inline void __attribute__((unused)) lor_map_cleanup_(void *map_ref) {
    lor_map_deinit(map_ref);
}
#define LOR_AUTO_MAP __attribute__((cleanup(lor_map_cleanup_)))
#else
#define LOR_AUTO_MAP
#endif

#ifdef __cplusplus
}
#endif

#endif
