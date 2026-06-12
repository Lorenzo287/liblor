// SPDX-License-Identifier: MIT

#ifndef LOR_STRING_H
#define LOR_STRING_H

#include <stddef.h>
#include <stdio.h>

#include "lor/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Borrowed byte string.

   A view does not own `data`, does not require a NUL terminator, and remains
   valid only while the referenced bytes remain valid and unmoved. `data` may
   be `NULL` only when `size` is zero. */
typedef struct LorStringView {
    const char *data;
    size_t size;
} LorStringView;

#define LOR_STRING_VIEW_INIT {NULL, 0u}
#define LOR_SV_LITERAL(text) {(text), sizeof(text) - 1u}

// Creates a view over `size` bytes. Invalid `NULL`/non-zero input returns empty.
LorStringView lor_sv_from_parts(const char *data, size_t size);

// Creates a view over a NUL-terminated C string. Passing `NULL` returns empty.
LorStringView lor_sv_from_cstr(const char *text);

// Returns non-zero when the view satisfies its pointer/size invariant.
int lor_sv_is_valid(LorStringView view);

// Returns non-zero when `view` contains no bytes.
int lor_sv_is_empty(LorStringView view);

// Compares the complete binary contents of two views.
int lor_sv_equal(LorStringView a, LorStringView b);

// Returns whether `view` begins with `prefix`.
int lor_sv_starts_with(LorStringView view, LorStringView prefix);

// Returns whether `view` ends with `suffix`.
int lor_sv_ends_with(LorStringView view, LorStringView suffix);

// Removes ASCII whitespace from the left side of `view`.
LorStringView lor_sv_trim_left(LorStringView view);

// Removes ASCII whitespace from the right side of `view`.
LorStringView lor_sv_trim_right(LorStringView view);

// Removes ASCII whitespace from both sides of `view`.
LorStringView lor_sv_trim(LorStringView view);

/* Returns at most `size` bytes starting at `offset`.

   Out-of-range offsets and sizes are clamped to the view bounds. */
LorStringView lor_sv_slice(LorStringView view, size_t offset, size_t size);

// Returns at most the first `size` bytes of `view`.
LorStringView lor_sv_take_left(LorStringView view, size_t size);

// Returns at most the last `size` bytes of `view`.
LorStringView lor_sv_take_right(LorStringView view, size_t size);

// Removes and returns at most the first `size` bytes from `view`.
LorStringView lor_sv_chop_left(LorStringView *view, size_t size);

// Removes and returns at most the last `size` bytes from `view`.
LorStringView lor_sv_chop_right(LorStringView *view, size_t size);

/* Finds `needle` and writes its byte offset to `index`.

   An empty needle is found at offset zero. `index` is unchanged on failure. */
int lor_sv_find(LorStringView view, LorStringView needle, size_t *index);

// Finds `needle` and writes its byte offset to `index`.
int lor_sv_find_char(LorStringView view, char needle, size_t *index);

/* Splits `view` around the first non-empty `delimiter`.

   On success, writes the bytes before and after the delimiter and returns
   non-zero. When not found, writes `view` to `before`, an empty view to
   `after`, and returns zero. */
int lor_sv_split(LorStringView view, LorStringView delimiter,
                      LorStringView *before, LorStringView *after);

// Character-delimiter form of `lor_sv_split`.
int lor_sv_split_char(LorStringView view, char delimiter, LorStringView *before,
                           LorStringView *after);

/* Removes the next delimiter-separated part from `view`.

   When the delimiter is found, consumes it and returns non-zero. Otherwise,
   returns the remaining input as `part`, empties `view`, and returns zero.
   An empty or invalid delimiter leaves `view` unchanged. */
int lor_sv_chop(LorStringView *view, LorStringView delimiter, LorStringView *part);

// Character-delimiter form of `lor_sv_chop`.
int lor_sv_chop_char(LorStringView *view, char delimiter, LorStringView *part);

/* Writes exactly `view.size` bytes to `out` without adding a newline.

   Returns non-zero when the complete view was written. Unlike `%s` and
   `%.*s`, this preserves embedded NUL bytes and supports lengths above
   `INT_MAX`. */
int lor_sv_fprint(FILE *out, LorStringView view);

// Writes `view` to stdout using `lor_sv_fprint`.
int lor_sv_print(LorStringView view);

/* Owned, mutable byte string.

   The handle points directly to NUL-terminated content and can be indexed or
   passed to read-only C string APIs when non-NULL. Size and capacity metadata
   are stored in a private allocation header. Do not free the handle directly;
   release it with `lor_string_deinit`. */
typedef char *LorString;

#define LOR_STRING_INIT NULL

// Releases owned storage and resets `string` to `LOR_STRING_INIT`.
void lor_string_deinit(LorString *string);

/* Scope-exit cleanup for `LorString` on GCC and Clang.

   Unsupported compilers leave `LOR_AUTO_STRING` empty, so explicit
   `lor_string_deinit` remains required for portable ownership paths. */
#if defined(__GNUC__) || defined(__clang__)
static inline void __attribute__((unused)) lor_string_cleanup_(LorString *string) {
    lor_string_deinit(string);
}
#define LOR_AUTO_STRING __attribute__((cleanup(lor_string_cleanup_)))
#else
#define LOR_AUTO_STRING
#endif

// Removes all contents while retaining allocated capacity.
void lor_string_clear(LorString string);

// Returns the number of content bytes, excluding the trailing NUL.
size_t lor_string_size(LorString string);

// Returns the writable content capacity, excluding the trailing NUL.
size_t lor_string_capacity(LorString string);

// Returns a borrowed view of `string`, or an empty view for `NULL`.
LorStringView lor_string_view(LorString string);

// Returns a stable empty C string when `string` is `NULL`.
const char *lor_string_cstr(LorString string);

/* Ensures room for at least `capacity` content bytes.

   The string is unchanged on failure. */
LorStatus lor_string_reserve(LorString *string, size_t capacity);

/* Replaces the contents with `view`, allocating when `string` is empty.

   The string is unchanged on failure. */
LorStatus lor_string_assign(LorString *string, LorStringView view);

// NUL-terminated C-string form of `lor_string_assign`.
LorStatus lor_string_assign_cstr(LorString *string, const char *text);

/* Appends `view`, allocating when `string` is empty.

   The string is unchanged on failure. */
LorStatus lor_string_append(LorString *string, LorStringView view);

// NUL-terminated C-string form of `lor_string_append`.
LorStatus lor_string_append_cstr(LorString *string, const char *text);

// Appends one byte. The string is unchanged on failure.
LorStatus lor_string_append_char(LorString *string, char value);

#ifdef __cplusplus
}
#endif

#endif
