// SPDX-License-Identifier: MIT

#include "lor/array.h"

#if defined(LOR_LEAKCHECK)
#define LOR_MEMORY_NO_STDLIB_MACROS
#include "lor/memory.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
typedef max_align_t LorArrayAlignment;
#else
typedef union LorArrayAlignment {
    void *pointer;
    long double long_double;
    long long long_long;
} LorArrayAlignment;
#endif

typedef union LorArrayHeader {
    struct {
        size_t size;
        size_t capacity;
        size_t element_size;
    } values;
    LorArrayAlignment alignment;
} LorArrayHeader;

static LorArrayHeader *lor_array__header(void *array) {
    return (LorArrayHeader *)array - 1;
}

static const LorArrayHeader *lor_array__header_const(const void *array) {
    return (const LorArrayHeader *)array - 1;
}

static void *lor_array__read_ref(const void *array_ref) {
    void *array = NULL;
    if (array_ref != NULL) memcpy(&array, array_ref, sizeof(array));
    return array;
}

static void lor_array__write_ref(void *array_ref, void *array) {
    memcpy(array_ref, &array, sizeof(array));
}

static void *lor_array__realloc(void *ptr, size_t size) {
#if defined(LOR_LEAKCHECK)
    return lor_realloc_debug(ptr, size, __FILE__, __LINE__);
#else
    return realloc(ptr, size);
#endif
}

static void lor_array__free(void *ptr) {
#if defined(LOR_LEAKCHECK)
    lor_free_debug(ptr, __FILE__, __LINE__);
#else
    free(ptr);
#endif
}

static int lor_array__valid_element_size(const void *array,
                                         size_t element_size) {
    if (element_size == 0) return 0;
    return array == NULL ||
           lor_array__header_const(array)->values.element_size == element_size;
}

static LorStatus lor_array__growth_capacity(size_t current, size_t required,
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

/* Returns one for a valid alias, zero for no alias, and negative one when the
   source begins inside initialized array storage but extends beyond it. */
static int lor_array__source_offset(const void *array, const void *source,
                                    size_t byte_count, size_t *offset) {
    if (array == NULL || source == NULL) return 0;

    const LorArrayHeader *header = lor_array__header_const(array);
    if (header->values.capacity > SIZE_MAX / header->values.element_size)
        return -1;

    size_t initialized_bytes =
        header->values.size * header->values.element_size;
    size_t allocated_bytes =
        header->values.capacity * header->values.element_size;
    uintptr_t base = (uintptr_t)array;
    uintptr_t start = (uintptr_t)source;
    if (allocated_bytes > UINTPTR_MAX - base) return 0;

    uintptr_t allocation_end = base + allocated_bytes;
    if (start < base || start > allocation_end) return 0;

    size_t result = (size_t)(start - base);
    if (result > initialized_bytes) return -1;
    if (byte_count > initialized_bytes - result) return -1;
    if (offset != NULL) *offset = result;
    return 1;
}

size_t lor_array_size(const void *array) {
    return array != NULL ? lor_array__header_const(array)->values.size : 0;
}

size_t lor_array_capacity(const void *array) {
    return array != NULL ? lor_array__header_const(array)->values.capacity : 0;
}

size_t lor_array_element_size(const void *array) {
    return array != NULL
               ? lor_array__header_const(array)->values.element_size
               : 0;
}

LorStatus lor_array_reserve_raw(void *array_ref, size_t element_size,
                                size_t capacity) {
    if (array_ref == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    void *array = lor_array__read_ref(array_ref);
    if (!lor_array__valid_element_size(array, element_size))
        return LOR_STATUS_INVALID_ARGUMENT;

    size_t current_capacity = lor_array_capacity(array);
    if (capacity <= current_capacity) return LOR_STATUS_OK;

    size_t new_capacity = 0;
    LorStatus status =
        lor_array__growth_capacity(current_capacity, capacity, &new_capacity);
    if (status != LOR_STATUS_OK) return status;

    if (new_capacity > (SIZE_MAX - sizeof(LorArrayHeader)) / element_size)
        return LOR_STATUS_OVERFLOW;

    size_t allocation_size =
        sizeof(LorArrayHeader) + new_capacity * element_size;
    LorArrayHeader *header =
        array != NULL ? lor_array__header(array) : NULL;
    LorArrayHeader *new_header =
        (LorArrayHeader *)lor_array__realloc(header, allocation_size);
    if (new_header == NULL) return LOR_STATUS_OUT_OF_MEMORY;

    if (array == NULL) {
        new_header->values.size = 0;
        new_header->values.element_size = element_size;
    }
    new_header->values.capacity = new_capacity;
    lor_array__write_ref(array_ref, new_header + 1);
    return LOR_STATUS_OK;
}

LorStatus lor_array_resize_raw(void *array_ref, size_t element_size,
                               size_t size) {
    if (array_ref == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    void *array = lor_array__read_ref(array_ref);
    if (!lor_array__valid_element_size(array, element_size))
        return LOR_STATUS_INVALID_ARGUMENT;

    size_t old_size = lor_array_size(array);
    if (size > old_size) {
        LorStatus status =
            lor_array_reserve_raw(array_ref, element_size, size);
        if (status != LOR_STATUS_OK) return status;

        array = lor_array__read_ref(array_ref);
        memset((unsigned char *)array + old_size * element_size, 0,
               (size - old_size) * element_size);
    }

    if (array != NULL)
        lor_array__header(array)->values.size = size;
    return LOR_STATUS_OK;
}

LorStatus lor_array_append_raw(void *array_ref, size_t element_size,
                               const void *elements, size_t count) {
    if (array_ref == NULL || (elements == NULL && count != 0))
        return LOR_STATUS_INVALID_ARGUMENT;

    void *array = lor_array__read_ref(array_ref);
    if (!lor_array__valid_element_size(array, element_size))
        return LOR_STATUS_INVALID_ARGUMENT;
    if (count == 0) return LOR_STATUS_OK;
    if (count > SIZE_MAX / element_size) return LOR_STATUS_OVERFLOW;

    size_t old_size = lor_array_size(array);
    if (count > SIZE_MAX - old_size) return LOR_STATUS_OVERFLOW;

    size_t byte_count = count * element_size;
    size_t source_offset = 0;
    int alias =
        lor_array__source_offset(array, elements, byte_count, &source_offset);
    if (alias < 0) return LOR_STATUS_INVALID_ARGUMENT;

    size_t new_size = old_size + count;
    LorStatus status =
        lor_array_reserve_raw(array_ref, element_size, new_size);
    if (status != LOR_STATUS_OK) return status;

    array = lor_array__read_ref(array_ref);
    if (alias > 0)
        elements = (const unsigned char *)array + source_offset;

    memmove((unsigned char *)array + old_size * element_size, elements,
            byte_count);
    lor_array__header(array)->values.size = new_size;
    return LOR_STATUS_OK;
}

LorStatus lor_array_append_array_raw(void *array_ref, size_t element_size,
                                     const void *source,
                                     size_t source_element_size) {
    if (element_size == 0 || source_element_size != element_size)
        return LOR_STATUS_INVALID_ARGUMENT;
    if (source != NULL &&
        lor_array_element_size(source) != source_element_size)
        return LOR_STATUS_INVALID_ARGUMENT;

    return lor_array_append_raw(array_ref, element_size, source,
                                lor_array_size(source));
}

LorStatus lor_array_insert_raw(void *array_ref, size_t element_size,
                               size_t index, const void *elements,
                               size_t count) {
    if (array_ref == NULL || (elements == NULL && count != 0))
        return LOR_STATUS_INVALID_ARGUMENT;

    void *array = lor_array__read_ref(array_ref);
    if (!lor_array__valid_element_size(array, element_size))
        return LOR_STATUS_INVALID_ARGUMENT;

    size_t old_size = lor_array_size(array);
    if (index > old_size) return LOR_STATUS_INVALID_ARGUMENT;
    if (count == 0) return LOR_STATUS_OK;
    if (count > SIZE_MAX / element_size) return LOR_STATUS_OVERFLOW;
    if (count > SIZE_MAX - old_size) return LOR_STATUS_OVERFLOW;

    size_t byte_count = count * element_size;
    size_t source_offset = 0;
    int alias =
        lor_array__source_offset(array, elements, byte_count, &source_offset);
    if (alias < 0) return LOR_STATUS_INVALID_ARGUMENT;

    void *copy = NULL;
    if (alias > 0) {
        copy = lor_array__realloc(NULL, byte_count);
        if (copy == NULL) return LOR_STATUS_OUT_OF_MEMORY;
        memcpy(copy, (const unsigned char *)array + source_offset,
               byte_count);
        elements = copy;
    }

    LorStatus status =
        lor_array_reserve_raw(array_ref, element_size, old_size + count);
    if (status != LOR_STATUS_OK) {
        lor_array__free(copy);
        return status;
    }

    array = lor_array__read_ref(array_ref);
    unsigned char *destination =
        (unsigned char *)array + index * element_size;
    size_t tail_count = old_size - index;
    if (tail_count != 0)
        memmove(destination + byte_count, destination,
                tail_count * element_size);
    memcpy(destination, elements, byte_count);
    lor_array__header(array)->values.size = old_size + count;
    lor_array__free(copy);
    return LOR_STATUS_OK;
}

LorStatus lor_array_shrink_to_fit_raw(void *array_ref, size_t element_size) {
    if (array_ref == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    void *array = lor_array__read_ref(array_ref);
    if (!lor_array__valid_element_size(array, element_size))
        return LOR_STATUS_INVALID_ARGUMENT;
    if (array == NULL) return LOR_STATUS_OK;

    LorArrayHeader *header = lor_array__header(array);
    size_t size = header->values.size;
    if (size == header->values.capacity) return LOR_STATUS_OK;

    if (size == 0) {
        lor_array__free(header);
        lor_array__write_ref(array_ref, NULL);
        return LOR_STATUS_OK;
    }

    if (size > (SIZE_MAX - sizeof(LorArrayHeader)) / element_size)
        return LOR_STATUS_OVERFLOW;

    size_t allocation_size = sizeof(LorArrayHeader) + size * element_size;
    LorArrayHeader *new_header =
        (LorArrayHeader *)lor_array__realloc(header, allocation_size);
    if (new_header == NULL) return LOR_STATUS_OUT_OF_MEMORY;

    new_header->values.capacity = size;
    lor_array__write_ref(array_ref, new_header + 1);
    return LOR_STATUS_OK;
}

void lor_array_clear(void *array) {
    if (array != NULL) lor_array__header(array)->values.size = 0;
}

int lor_array_pop(void *array, void *out_element) {
    if (array == NULL) return 0;

    LorArrayHeader *header = lor_array__header(array);
    if (header->values.size == 0) return 0;

    size_t index = header->values.size - 1u;
    unsigned char *element =
        (unsigned char *)array + index * header->values.element_size;
    if (out_element != NULL)
        memcpy(out_element, element, header->values.element_size);
    header->values.size = index;
    return 1;
}

int lor_array_remove(void *array, size_t index, void *out_element) {
    if (array == NULL) return 0;

    LorArrayHeader *header = lor_array__header(array);
    if (index >= header->values.size) return 0;

    size_t element_size = header->values.element_size;
    unsigned char *element = (unsigned char *)array + index * element_size;
    if (out_element != NULL) memcpy(out_element, element, element_size);

    size_t remaining = header->values.size - index - 1u;
    if (remaining != 0)
        memmove(element, element + element_size, remaining * element_size);
    header->values.size -= 1u;
    return 1;
}

int lor_array_remove_range(void *array, size_t index, size_t count) {
    if (array == NULL) return 0;

    LorArrayHeader *header = lor_array__header(array);
    if (index > header->values.size ||
        count > header->values.size - index)
        return 0;
    if (count == 0) return 1;

    size_t element_size = header->values.element_size;
    size_t remaining = header->values.size - index - count;
    if (remaining != 0)
        memmove((unsigned char *)array + index * element_size,
                (unsigned char *)array + (index + count) * element_size,
                remaining * element_size);
    header->values.size -= count;
    return 1;
}

int lor_array_remove_unordered(void *array, size_t index, void *out_element) {
    if (array == NULL) return 0;

    LorArrayHeader *header = lor_array__header(array);
    if (index >= header->values.size) return 0;

    size_t element_size = header->values.element_size;
    unsigned char *element = (unsigned char *)array + index * element_size;
    if (out_element != NULL) memcpy(out_element, element, element_size);

    size_t last = header->values.size - 1u;
    if (index != last)
        memcpy(element, (unsigned char *)array + last * element_size,
               element_size);
    header->values.size = last;
    return 1;
}

void lor_array_deinit(void *array_ref) {
    if (array_ref == NULL) return;

    void *array = lor_array__read_ref(array_ref);
    if (array != NULL) lor_array__free(lor_array__header(array));
    lor_array__write_ref(array_ref, NULL);
}
