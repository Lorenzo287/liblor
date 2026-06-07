// SPDX-License-Identifier: MIT

#include "lor/string.h"

#if defined(LOR_LEAKCHECK)
#define LOR_MEMORY_NO_STDLIB_MACROS
#include "lor/memory.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int lor_sv__ascii_space(unsigned char value) {
    return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
           value == '\f' || value == '\v';
}

static const char *lor_sv__offset(LorStringView view, size_t offset) {
    return view.data != NULL ? view.data + offset : NULL;
}

LorStringView lor_sv_from_parts(const char *data, size_t size) {
    if (data == NULL && size != 0) return (LorStringView)LOR_STRING_VIEW_INIT;
    return (LorStringView){.data = data, .size = size};
}

LorStringView lor_sv_from_cstr(const char *text) {
    if (text == NULL) return (LorStringView)LOR_STRING_VIEW_INIT;
    return lor_sv_from_parts(text, strlen(text));
}

int lor_sv_is_valid(LorStringView view) {
    return view.data != NULL || view.size == 0;
}

int lor_sv_is_empty(LorStringView view) {
    return view.size == 0;
}

int lor_sv_equal(LorStringView a, LorStringView b) {
    if (!lor_sv_is_valid(a) || !lor_sv_is_valid(b) || a.size != b.size) return 0;
    if (a.size == 0) return 1;
    return memcmp(a.data, b.data, a.size) == 0;
}

int lor_sv_starts_with(LorStringView view, LorStringView prefix) {
    if (!lor_sv_is_valid(view) || !lor_sv_is_valid(prefix) ||
        prefix.size > view.size)
        return 0;

    return lor_sv_equal(lor_sv_take_left(view, prefix.size), prefix);
}

int lor_sv_ends_with(LorStringView view, LorStringView suffix) {
    if (!lor_sv_is_valid(view) || !lor_sv_is_valid(suffix) ||
        suffix.size > view.size)
        return 0;

    return lor_sv_equal(lor_sv_take_right(view, suffix.size), suffix);
}

LorStringView lor_sv_trim_left(LorStringView view) {
    if (!lor_sv_is_valid(view)) return (LorStringView)LOR_STRING_VIEW_INIT;

    size_t offset = 0;
    while (offset < view.size &&
           lor_sv__ascii_space((unsigned char)view.data[offset]))
        offset += 1u;

    return lor_sv_slice(view, offset, view.size - offset);
}

LorStringView lor_sv_trim_right(LorStringView view) {
    if (!lor_sv_is_valid(view)) return (LorStringView)LOR_STRING_VIEW_INIT;

    size_t size = view.size;
    while (size > 0 && lor_sv__ascii_space((unsigned char)view.data[size - 1u]))
        size -= 1u;

    return lor_sv_slice(view, 0, size);
}

LorStringView lor_sv_trim(LorStringView view) {
    return lor_sv_trim_right(lor_sv_trim_left(view));
}

LorStringView lor_sv_slice(LorStringView view, size_t offset, size_t size) {
    if (!lor_sv_is_valid(view)) return (LorStringView)LOR_STRING_VIEW_INIT;
    if (offset > view.size) offset = view.size;
    if (size > view.size - offset) size = view.size - offset;
    return lor_sv_from_parts(lor_sv__offset(view, offset), size);
}

LorStringView lor_sv_take_left(LorStringView view, size_t size) {
    return lor_sv_slice(view, 0, size);
}

LorStringView lor_sv_take_right(LorStringView view, size_t size) {
    if (!lor_sv_is_valid(view)) return (LorStringView)LOR_STRING_VIEW_INIT;
    if (size > view.size) size = view.size;
    return lor_sv_slice(view, view.size - size, size);
}

LorStringView lor_sv_chop_left(LorStringView *view, size_t size) {
    if (view == NULL || !lor_sv_is_valid(*view))
        return (LorStringView)LOR_STRING_VIEW_INIT;

    LorStringView result = lor_sv_take_left(*view, size);
    *view = lor_sv_slice(*view, result.size, view->size - result.size);
    return result;
}

LorStringView lor_sv_chop_right(LorStringView *view, size_t size) {
    if (view == NULL || !lor_sv_is_valid(*view))
        return (LorStringView)LOR_STRING_VIEW_INIT;

    LorStringView result = lor_sv_take_right(*view, size);
    view->size -= result.size;
    return result;
}

int lor_sv_find(LorStringView view, LorStringView needle, size_t *index) {
    if (!lor_sv_is_valid(view) || !lor_sv_is_valid(needle) ||
        needle.size > view.size)
        return 0;

    if (needle.size == 0) {
        if (index != NULL) *index = 0;
        return 1;
    }

    size_t limit = view.size - needle.size;
    for (size_t i = 0; i <= limit; ++i) {
        if (view.data[i] == needle.data[0] &&
            memcmp(view.data + i, needle.data, needle.size) == 0) {
            if (index != NULL) *index = i;
            return 1;
        }
    }

    return 0;
}

int lor_sv_find_char(LorStringView view, char needle, size_t *index) {
    if (!lor_sv_is_valid(view)) return 0;

    const char *found = view.size != 0 ? memchr(view.data, needle, view.size) : NULL;
    if (found == NULL) return 0;
    if (index != NULL) *index = (size_t)(found - view.data);
    return 1;
}

int lor_sv_split_once(LorStringView view, LorStringView delimiter,
                      LorStringView *before, LorStringView *after) {
    size_t index = 0;
    int found = delimiter.size != 0 && lor_sv_find(view, delimiter, &index);

    if (before != NULL) *before = found ? lor_sv_take_left(view, index) : view;
    if (after != NULL) {
        *after = found
                     ? lor_sv_slice(view, index + delimiter.size,
                                    view.size - index - delimiter.size)
                     : (LorStringView)LOR_STRING_VIEW_INIT;
    }

    return found;
}

int lor_sv_split_once_char(LorStringView view, char delimiter,
                           LorStringView *before, LorStringView *after) {
    return lor_sv_split_once(view, lor_sv_from_parts(&delimiter, 1), before,
                             after);
}

int lor_sv_chop(LorStringView *view, LorStringView delimiter,
                LorStringView *part) {
    if (view == NULL || !lor_sv_is_valid(*view) ||
        !lor_sv_is_valid(delimiter) || delimiter.size == 0) {
        if (part != NULL) *part = (LorStringView)LOR_STRING_VIEW_INIT;
        return 0;
    }

    LorStringView before;
    LorStringView after;
    int found = lor_sv_split_once(*view, delimiter, &before, &after);
    if (part != NULL) *part = before;

    if (found) {
        *view = after;
    } else {
        *view = lor_sv_slice(*view, view->size, 0);
    }

    return found;
}

int lor_sv_chop_char(LorStringView *view, char delimiter, LorStringView *part) {
    return lor_sv_chop(view, lor_sv_from_parts(&delimiter, 1), part);
}

int lor_sv_fprint(FILE *out, LorStringView view) {
    if (out == NULL || !lor_sv_is_valid(view)) return 0;
    if (view.size == 0) return 1;
    return fwrite(view.data, 1, view.size, out) == view.size;
}

int lor_sv_print(LorStringView view) {
    return lor_sv_fprint(stdout, view);
}

typedef struct LorStringHeader {
    size_t size;
    size_t capacity;
    char data[];
} LorStringHeader;

static LorStringHeader *lor_string__header(LorString string) {
    return (LorStringHeader *)((unsigned char *)string -
                               offsetof(LorStringHeader, data));
}

static const LorStringHeader *lor_string__header_const(LorString string) {
    return (const LorStringHeader *)((const unsigned char *)string -
                                     offsetof(LorStringHeader, data));
}

static void *lor_string__realloc(void *ptr, size_t size) {
#if defined(LOR_LEAKCHECK)
    return lor_realloc_debug(ptr, size, __FILE__, __LINE__);
#else
    return realloc(ptr, size);
#endif
}

static void lor_string__free(void *ptr) {
#if defined(LOR_LEAKCHECK)
    lor_free_debug(ptr, __FILE__, __LINE__);
#else
    free(ptr);
#endif
}

/* Returns one for a valid alias, zero for no alias, and negative one when a
   view begins inside the string but extends beyond its initialized bytes. */
static int lor_string__view_offset(LorString string, LorStringView view,
                                   size_t *offset) {
    if (string == NULL || view.data == NULL) return 0;

    size_t string_size = lor_string__header_const(string)->size;
    uintptr_t base = (uintptr_t)(const void *)string;
    uintptr_t start = (uintptr_t)(const void *)view.data;
    if (string_size > UINTPTR_MAX - base) return 0;

    uintptr_t end = base + string_size;
    if (start < base || start > end) return 0;

    size_t result = (size_t)(start - base);
    if (view.size > string_size - result) return -1;
    if (offset != NULL) *offset = result;
    return 1;
}

static LorStatus lor_string__growth_capacity(size_t current, size_t required,
                                             size_t *capacity) {
    if (required == SIZE_MAX) return LOR_STATUS_OVERFLOW;

    size_t result = current < 16u ? 16u : current;
    while (result < required) {
        if (result > SIZE_MAX / 2u) {
            result = required;
            break;
        }
        result *= 2u;
    }

    if (result == SIZE_MAX) return LOR_STATUS_OVERFLOW;
    *capacity = result;
    return LOR_STATUS_OK;
}

void lor_string_init(LorString *string) {
    if (string != NULL) *string = LOR_STRING_INIT;
}

LorStatus lor_string_init_view(LorString *string, LorStringView view) {
    if (string == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    *string = LOR_STRING_INIT;
    return lor_string_assign(string, view);
}

LorStatus lor_string_init_cstr(LorString *string, const char *text) {
    if (string == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    *string = LOR_STRING_INIT;
    if (text == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    return lor_string_assign(string, lor_sv_from_cstr(text));
}

void lor_string_deinit(LorString *string) {
    if (string == NULL) return;
    if (*string != NULL) lor_string__free(lor_string__header(*string));
    *string = LOR_STRING_INIT;
}

void lor_string_clear(LorString string) {
    if (string == NULL) return;
    lor_string__header(string)->size = 0;
    string[0] = '\0';
}

size_t lor_string_size(LorString string) {
    return string != NULL ? lor_string__header_const(string)->size : 0;
}

size_t lor_string_capacity(LorString string) {
    return string != NULL ? lor_string__header_const(string)->capacity : 0;
}

LorStringView lor_string_view(LorString string) {
    return lor_sv_from_parts(string, lor_string_size(string));
}

const char *lor_string_cstr(LorString string) {
    return string != NULL ? string : "";
}

LorStatus lor_string_reserve(LorString *string, size_t capacity) {
    if (string == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    size_t current_capacity = lor_string_capacity(*string);
    if (capacity <= current_capacity) return LOR_STATUS_OK;

    size_t new_capacity = 0;
    LorStatus status =
        lor_string__growth_capacity(current_capacity, capacity, &new_capacity);
    if (status != LOR_STATUS_OK) return status;

    size_t header_size = offsetof(LorStringHeader, data);
    if (new_capacity > SIZE_MAX - header_size - 1u)
        return LOR_STATUS_OVERFLOW;

    size_t size = lor_string_size(*string);
    LorStringHeader *header =
        *string != NULL ? lor_string__header(*string) : NULL;
    LorStringHeader *new_header = (LorStringHeader *)lor_string__realloc(
        header, header_size + new_capacity + 1u);
    if (new_header == NULL) return LOR_STATUS_OUT_OF_MEMORY;

    new_header->size = size;
    new_header->capacity = new_capacity;
    new_header->data[size] = '\0';
    *string = new_header->data;
    return LOR_STATUS_OK;
}

LorStatus lor_string_assign(LorString *string, LorStringView view) {
    if (string == NULL || !lor_sv_is_valid(view))
        return LOR_STATUS_INVALID_ARGUMENT;

    size_t offset = 0;
    int alias = lor_string__view_offset(*string, view, &offset);
    if (alias < 0) return LOR_STATUS_INVALID_ARGUMENT;

    LorStatus status = lor_string_reserve(string, view.size);
    if (status != LOR_STATUS_OK) return status;
    if (alias > 0) view.data = *string + offset;

    if (view.size != 0) memmove(*string, view.data, view.size);
    if (*string != NULL) {
        lor_string__header(*string)->size = view.size;
        (*string)[view.size] = '\0';
    }
    return LOR_STATUS_OK;
}

LorStatus lor_string_assign_cstr(LorString *string, const char *text) {
    if (text == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    return lor_string_assign(string, lor_sv_from_cstr(text));
}

LorStatus lor_string_append(LorString *string, LorStringView view) {
    if (string == NULL || !lor_sv_is_valid(view))
        return LOR_STATUS_INVALID_ARGUMENT;
    if (view.size == 0) return LOR_STATUS_OK;

    size_t old_size = lor_string_size(*string);
    if (view.size > SIZE_MAX - old_size) return LOR_STATUS_OVERFLOW;

    size_t offset = 0;
    int alias = lor_string__view_offset(*string, view, &offset);
    if (alias < 0) return LOR_STATUS_INVALID_ARGUMENT;

    size_t new_size = old_size + view.size;
    LorStatus status = lor_string_reserve(string, new_size);
    if (status != LOR_STATUS_OK) return status;
    if (alias > 0) view.data = *string + offset;

    memmove(*string + old_size, view.data, view.size);
    lor_string__header(*string)->size = new_size;
    (*string)[new_size] = '\0';
    return LOR_STATUS_OK;
}

LorStatus lor_string_append_cstr(LorString *string, const char *text) {
    if (text == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    return lor_string_append(string, lor_sv_from_cstr(text));
}

LorStatus lor_string_append_char(LorString *string, char value) {
    return lor_string_append(string, lor_sv_from_parts(&value, 1));
}
