// SPDX-License-Identifier: MIT

#ifndef LOR_SET_H
#define LOR_SET_H

#include <stddef.h>

#include "lor/features.h"
#include "lor/map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typed hash set.

   A set handle is an ordinary pointer to its key type. Keys are stored densely
   for direct indexing and iteration; a hidden map index provides lookup. */
#define LOR_SET_INIT NULL

typedef LorMapConfig LorSetConfig;
typedef LorMapKeyOwnership LorSetKeyOwnership;

#define LOR_SET_KEY_BORROWED LOR_MAP_KEY_BORROWED
#define LOR_SET_KEY_OWNED LOR_MAP_KEY_OWNED

// Returns byte hashing/equality with shallow key copies.
LorSetConfig lor_set_config_bytes(void);

// Returns borrowed or owned content semantics for `LorStringView` keys.
LorSetConfig lor_set_config_string_view(LorSetKeyOwnership ownership);

// Initializes an empty set with custom key behavior.
LorStatus lor_set_init_raw(void *set_ref, size_t element_size, LorSetConfig config);

// Returns the number of keys, or zero for `NULL`.
size_t lor_set_size(const void *set);

// Returns the allocated key capacity, or zero for `NULL`.
size_t lor_set_capacity(const void *set);

// Ensures room for at least `capacity` keys.
LorStatus lor_set_reserve_raw(void *set_ref, size_t element_size, size_t capacity);

// Adds `key`; adding an existing key is successful and leaves one copy.
LorStatus lor_set_add_raw(void *set_ref, size_t element_size, const void *key);

// Returns non-zero when `key` is present.
int lor_set_contains_raw(const void *set, size_t element_size, const void *key);

// Removes `key` when present and returns non-zero.
int lor_set_remove_raw(void *set, size_t element_size, const void *key);

// Removes all keys and owned key resources while retaining capacity/config.
void lor_set_clear(void *set);

// Releases all storage and resets the handle to `NULL`.
void lor_set_deinit(void *set_ref);

/* Creates a new result set.

   `result_ref` must point to a `NULL` handle. The result inherits the key
   policy of a non-empty operand, or otherwise the first initialized operand.
   Compatible owning policies take precedence so results remain independent.
   Two non-empty operands must use matching hash/equality semantics. On
   failure, `result_ref` remains `NULL`. */
LorStatus lor_set_union_raw(void *result_ref, size_t element_size, const void *a,
                            const void *b);
LorStatus lor_set_intersection_raw(void *result_ref, size_t element_size,
                                   const void *a, const void *b);
LorStatus lor_set_difference_raw(void *result_ref, size_t element_size,
                                 const void *a, const void *b);
LorStatus lor_set_symmetric_difference_raw(void *result_ref, size_t element_size,
                                           const void *a, const void *b);

// Mathematical relationship predicates. Incompatible non-empty sets are false.
int lor_set_equal_raw(const void *a, const void *b, size_t element_size);
int lor_set_is_subset_raw(const void *a, const void *b, size_t element_size);
int lor_set_is_proper_subset_raw(const void *a, const void *b, size_t element_size);
int lor_set_is_superset_raw(const void *a, const void *b, size_t element_size);
int lor_set_is_proper_superset_raw(const void *a, const void *b,
                                   size_t element_size);
int lor_set_is_disjoint_raw(const void *a, const void *b, size_t element_size);

/* Typed portable operations.

   `lor_set_add`, `contains`, and `remove` take lvalues of the exact key type.
   In C, the `_as` forms construct C99 compound literals. */
#define lor_set_init(set, config) \
    lor_set_init_raw(&(set), sizeof *(set), (config))
#define lor_set_reserve(set, capacity) \
    lor_set_reserve_raw(&(set), sizeof *(set), (capacity))
#define lor_set_add(set, key_value) \
    lor_set_add_raw(&(set), sizeof *(set), &(key_value))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_set_add_as(set, type, ...) \
    lor_set_add_raw(&(set), sizeof *(set), &(type){__VA_ARGS__})
#endif
#define lor_set_contains(set, key_value) \
    lor_set_contains_raw((set), sizeof *(set), &(key_value))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_set_contains_as(set, type, ...) \
    lor_set_contains_raw((set), sizeof *(set), &(type){__VA_ARGS__})
#endif
#define lor_set_remove(set, key_value) \
    lor_set_remove_raw((set), sizeof *(set), &(key_value))
#if LOR_HAS_COMPOUND_LITERALS
#define lor_set_remove_as(set, type, ...) \
    lor_set_remove_raw((set), sizeof *(set), &(type){__VA_ARGS__})
#endif
#define lor_set_union(result, a, b) \
    lor_set_union_raw(&(result), sizeof *(result), (a), (b))
#define lor_set_intersection(result, a, b) \
    lor_set_intersection_raw(&(result), sizeof *(result), (a), (b))
#define lor_set_difference(result, a, b) \
    lor_set_difference_raw(&(result), sizeof *(result), (a), (b))
#define lor_set_symmetric_difference(result, a, b) \
    lor_set_symmetric_difference_raw(&(result), sizeof *(result), (a), (b))
#define lor_set_equal(a, b) \
    lor_set_equal_raw((a), (b), sizeof *(a))
#define lor_set_is_subset(a, b) \
    lor_set_is_subset_raw((a), (b), sizeof *(a))
#define lor_set_is_proper_subset(a, b) \
    lor_set_is_proper_subset_raw((a), (b), sizeof *(a))
#define lor_set_is_superset(a, b) \
    lor_set_is_superset_raw((a), (b), sizeof *(a))
#define lor_set_is_proper_superset(a, b) \
    lor_set_is_proper_superset_raw((a), (b), sizeof *(a))
#define lor_set_is_disjoint(a, b) \
    lor_set_is_disjoint_raw((a), (b), sizeof *(a))

#if LOR_HAS_TYPEOF && LOR_HAS_STATEMENT_EXPRESSIONS
#define LOR_HAS_SET_AUTO 1
#define lor_set_add_auto(set, key_value)                       \
    __extension__({                                            \
        lor_typeof(*(set)) lor_set__key = (key_value);         \
        lor_set_add_raw(&(set), sizeof *(set), &lor_set__key); \
    })
#define lor_set_contains_auto(set, key_value)                      \
    __extension__({                                                \
        lor_typeof(*(set)) lor_set__key = (key_value);             \
        lor_set_contains_raw((set), sizeof *(set), &lor_set__key); \
    })
#define lor_set_remove_auto(set, key_value)                      \
    __extension__({                                              \
        lor_typeof(*(set)) lor_set__key = (key_value);           \
        lor_set_remove_raw((set), sizeof *(set), &lor_set__key); \
    })
#else
#define LOR_HAS_SET_AUTO 0
#endif

/* Scope-exit cleanup for sets on GCC and Clang.

   Unsupported compilers leave `LOR_AUTO_SET` empty, so explicit
   `lor_set_deinit` remains required for portable ownership paths. */
#if defined(__GNUC__) || defined(__clang__)
static inline void __attribute__((unused)) lor_set_cleanup_(void *set_ref) {
    lor_set_deinit(set_ref);
}
#define LOR_AUTO_SET __attribute__((cleanup(lor_set_cleanup_)))
#else
#define LOR_AUTO_SET
#endif

#ifdef __cplusplus
}
#endif

#endif
