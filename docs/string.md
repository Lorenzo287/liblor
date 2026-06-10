# Strings

liblor supports standard C strings, borrowed string views, and owned dynamic
strings as complementary representations.

## String Views

`LorStringView` is a pointer plus byte length. It does not allocate, own its
bytes, or require NUL termination. Views make parsing, slicing, trimming,
searching, and delimiter handling possible without copying input.

```c
LorStringView input = lor_sv_from_cstr("name = liblor");
LorStringView key;
LorStringView value;

if (lor_sv_split_once_char(input, '=', &key, &value)) {
    key = lor_sv_trim(key);
    value = lor_sv_trim(value);
}
```

`lor_sv_is_valid(view)` returns non-zero when a view satisfies its pointer/size
invariant. `lor_sv_is_empty(view)` returns non-zero when a view contains no
bytes.

`lor_sv_equal(a, b)` compares the binary contents of two views.
`lor_sv_starts_with(view, prefix)` and `lor_sv_ends_with(view, suffix)` check
for presence at the bounds.

`lor_sv_take_left(view, size)` and `lor_sv_take_right(view, size)` return a
sub-view of the requested size. `lor_sv_chop_left(&view, size)` and
`lor_sv_chop_right(&view, size)` return the sub-view and update the original
view to point to the remaining bytes.

A view remains valid only while its source storage remains valid and unmoved.
In particular, views into a `LorString` must not be retained across operations
that may grow that string.

Whitespace trimming uses the ASCII whitespace bytes: space, tab, newline,
carriage return, form feed, and vertical tab. It is independent of locale.

Use `lor_sv_print(view)` for stdout or `lor_sv_fprint(stream, view)` for another
`FILE *`. These functions write exactly the bytes in the view, so they cannot
continue past a delimiter and do not have the `int` length limit of `%.*s`.
They do not add a newline.

## Owned Strings

`LorString` is an owned `char *` handle. Size and capacity are stored in a
private header immediately before the returned pointer. Its contents are
binary-safe, while an extra trailing NUL byte makes every non-`NULL` string
compatible with read-only C string APIs.

```c
LorString text = LOR_STRING_INIT;

if (lor_string_append_cstr(&text, "hello") == LOR_STATUS_OK &&
    lor_string_append_char(&text, ' ') == LOR_STATUS_OK &&
    lor_string_append_cstr(&text, "world") == LOR_STATUS_OK) {
    puts(text);
    printf("size: %zu\n", lor_string_size(text));
    printf("capacity: %zu\n", lor_string_capacity(text));
}

lor_string_deinit(&text);
```

Operations that may allocate take `LorString *` because growth can replace the
content pointer. Read-only operations, `lor_string_clear`, indexing, and C
string APIs use the handle directly. `lor_string_cstr(text)` returns a stable
empty C string when `text` is still `NULL`; use it when an empty string may be
passed to an API that does not accept `NULL`.

Do not pass a `LorString` to `free`, write beyond `lor_string_size(text)`, or
retain pointers and views across operations that may grow it. Release it with
`lor_string_deinit(&text)`, which also resets the handle to `NULL`.

On GCC and Clang, `LOR_AUTO_STRING` calls `lor_string_deinit` automatically at
scope exit:

```c
LOR_AUTO_STRING LorString text = LOR_STRING_INIT;
lor_string_append_cstr(&text, "cleaned up automatically");
```

On unsupported compilers the macro is empty, so use explicit deinitialization
when cleanup must be portable across compilers.

`LorStringView` deliberately exposes `data` and `size`, and `LorString` is a
`char *` typedef. Callers may initialize views with struct literals, inspect
their fields, perform careful pointer-based slicing, initialize an owned string
with `char *text = NULL`, and use non-`NULL` owned strings with read-only C
string APIs. See `examples/string_low_level.c`.

The dynamic string's prefix header is still private. Direct use must not call
`free`, change the stored length, write beyond the current size, or retain
pointers into the string across an operation that may grow it.
