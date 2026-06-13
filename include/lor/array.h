// SPDX-License-Identifier: MIT

#ifndef LOR_ARRAY_H
#define LOR_ARRAY_H

#include <stddef.h>

#include "lor/features.h"
#include "lor/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generic owned dynamic array.

   An array handle is an ordinary typed pointer initialized to `NULL`. Elements
   can be indexed directly, while size and capacity metadata live in a private
   prefix header. Operations that may grow the array receive the address of the
   handle so they can replace it after reallocation. */
#define LOR_ARRAY_INIT NULL

// Returns the number of initialized elements, or zero for `NULL`.
size_t lor_array_size(const void *array);

// Returns the allocated element capacity, or zero for `NULL`.
size_t lor_array_capacity(const void *array);

// Returns the stored element size, or zero for `NULL`.
size_t lor_array_element_size(const void *array);

/* Ensures capacity for at least `capacity` elements.

   `array_ref` must point to a typed array handle and `element_size` must equal
   `sizeof *array`. The original array is unchanged on failure. */
LorStatus lor_array_reserve_raw(void *array_ref, size_t element_size,
                                size_t capacity);

/* Changes the initialized element count.

   New elements are zero-initialized. Shrinking retains allocated capacity. */
LorStatus lor_array_resize_raw(void *array_ref, size_t element_size, size_t size);

/* Appends `count` elements copied from `elements`.

   `elements` may point into the same array; aliases remain valid across
   reallocation. Passing `NULL` is valid only when `count` is zero. */
LorStatus lor_array_append_raw(void *array_ref, size_t element_size,
                               const void *elements, size_t count);

/* Appends every element from another dynamic array.

   The source and destination element sizes must match. Self-append is valid. */
LorStatus lor_array_append_array_raw(void *array_ref, size_t element_size,
                                     const void *source, size_t source_element_size);

/* Inserts `count` copied elements before `index`.

   `index` may equal the current size to append. `elements` may point into the
   same array, including across the insertion point. */
LorStatus lor_array_insert_raw(void *array_ref, size_t element_size, size_t index,
                               const void *elements, size_t count);

// Reduces capacity to the current size. An empty array becomes `NULL`.
LorStatus lor_array_shrink_to_fit_raw(void *array_ref, size_t element_size);

// Removes all elements while retaining allocated capacity.
void lor_array_clear(void *array);

// Removes and optionally copies the last element. Returns zero when empty.
int lor_array_pop(void *array, void *out_element);

/* Removes the element at `index`, preserving order.

   The removed value is copied to `out_element` when non-NULL. */
int lor_array_remove(void *array, size_t index, void *out_element);

// Removes `count` elements from `index`, preserving order.
int lor_array_remove_range(void *array, size_t index, size_t count);

/* Removes the element at `index` by moving the last element into its slot.

   This is O(1) but does not preserve order. */
int lor_array_remove_unordered(void *array, size_t index, void *out_element);

// Releases owned storage and resets the handle to `NULL`.
void lor_array_deinit(void *array_ref);

/* Typed convenience operations.

   `lor_array_push` takes an lvalue of the exact element type so its address
   can be copied portably.
   In C, use `lor_array_push_as` for literals and inline aggregate
   initialization.
   When supported, `lor_array_push_auto` accepts any assignable expression and
   infers the destination element type with `lor_typeof`. */
#define lor_array_reserve(array, capacity) \
    lor_array_reserve_raw(&(array), sizeof *(array), (capacity))
#define lor_array_resize(array, size) \
    lor_array_resize_raw(&(array), sizeof *(array), (size))
#define lor_array_append(array, elements, count) \
    lor_array_append_raw(&(array), sizeof *(array), (elements), (count))
#define lor_array_append_array(array, source) \
    lor_array_append_array_raw(&(array), sizeof *(array), (source), sizeof *(source))
#define lor_array_push(array, value) \
    lor_array_append_raw(&(array), sizeof *(array), &(value), 1u)
#if LOR_HAS_COMPOUND_LITERALS
#define lor_array_push_as(array, type, ...) \
    lor_array_append_raw(&(array), sizeof *(array), &(type){__VA_ARGS__}, 1u)
#endif
#if LOR_HAS_TYPEOF && LOR_HAS_STATEMENT_EXPRESSIONS
#define LOR_HAS_ARRAY_PUSH_AUTO 1
#define lor_array_push_auto(array, value)                                       \
    __extension__({                                                             \
        lor_typeof(*(array)) lor_array__push_value = (value);                   \
        lor_array_append_raw(&(array), sizeof *(array), &lor_array__push_value, 1u); \
    })
#else
#define LOR_HAS_ARRAY_PUSH_AUTO 0
#endif
#define lor_array_insert_many(array, index, elements, count) \
    lor_array_insert_raw(&(array), sizeof *(array), (index), (elements), (count))
#define lor_array_insert(array, index, value) \
    lor_array_insert_raw(&(array), sizeof *(array), (index), &(value), 1u)
#if LOR_HAS_COMPOUND_LITERALS
#define lor_array_insert_as(array, index, type, ...)                               \
    lor_array_insert_raw(&(array), sizeof *(array), (index), &(type){__VA_ARGS__}, 1u)
#endif
#define lor_array_last(array) \
    (lor_array_size(array) != 0 ? &(array)[lor_array_size(array) - 1u] : NULL)
#define lor_array_shrink_to_fit(array) \
    lor_array_shrink_to_fit_raw(&(array), sizeof *(array))

/* Scope-exit cleanup for dynamic arrays on GCC and Clang.

   Unsupported compilers leave `LOR_AUTO_ARRAY` empty, so explicit
   `lor_array_deinit` remains required for portable ownership paths. */
#if defined(__GNUC__) || defined(__clang__)
static inline void __attribute__((unused)) lor_array_cleanup_(void *array_ref) {
    lor_array_deinit(array_ref);
}
#define LOR_AUTO_ARRAY __attribute__((cleanup(lor_array_cleanup_)))
#else
#define LOR_AUTO_ARRAY
#endif

#ifdef __cplusplus
}
#endif

#endif
