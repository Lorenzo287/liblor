# Strings

liblor supports standard C strings, borrowed string views, and owned dynamic
strings as complementary representations.

## Choosing A Representation

The three forms are intended to work as a pipeline rather than compete:

- Use C strings at interoperability boundaries such as `argv`, environment
  variables, libc, and operating-system APIs.
- Convert borrowed input to `LorStringView` for allocation-free parsing,
  searching, trimming, and slicing.
- Introduce `LorString` only when the program must own, replace, or extend
  bytes.
- Pass a non-`NULL` `LorString` directly to read-only C string APIs when the
  content is text without relevant embedded NUL bytes.

Views are the native byte-range input to owned-string operations because they
are more general than C strings: a view can represent a complete string, a
slice, non-NUL-terminated data, or bytes containing embedded NUL characters.
The `_cstr` functions are convenience forms for common NUL-terminated input:

```c
lor_string_append(&output, view);
lor_string_append_cstr(&output, text);
```

The second call is conceptually equivalent to converting `text` with
`lor_sv_from_cstr` and passing the resulting view.

A typical flow parses borrowed C input, builds an owned result, then returns to
a C API:

```c
LorStringView input = lor_sv_from_cstr("name = liblor");
LorStringView key;
LorStringView value;

if (!lor_sv_split_char(input, '=', &key, &value)) return;
value = lor_sv_trim(value);

LorString message = LOR_STRING_INIT;
if (lor_string_append_cstr(&message, "project: ") != LOR_STATUS_OK ||
    lor_string_append(&message, value) != LOR_STATUS_OK) {
    lor_string_deinit(&message);
    return;
}

puts(message);
lor_string_deinit(&message);
```

This keeps borrowed parsing cheap, allocates only for the new result, and
preserves ordinary C interoperability at both ends. See
`examples/string_interop.c` for a complete version of this pattern.

## String Views

`LorStringView` is a pointer plus byte length. It does not allocate, own its
bytes, or require NUL termination. Views make parsing, slicing, trimming,
searching, and delimiter handling possible without copying input.

```c
LorStringView input = lor_sv_from_cstr("name = liblor");
LorStringView key;
LorStringView value;

if (lor_sv_split_char(input, '=', &key, &value)) {
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

Initialize a new owner with `LOR_STRING_INIT`. No separate initialization
function is required. Both assignment and append allocate automatically when
the handle is empty:

```c
LorString assigned = LOR_STRING_INIT;
lor_string_assign_cstr(&assigned, "initial value");

LorString built = LOR_STRING_INIT;
lor_string_append_cstr(&built, "first");
lor_string_append_cstr(&built, " second");
```

`lor_string_assign` replaces the complete value, while `lor_string_append`
extends the existing value. Their `_cstr` forms accept NUL-terminated input;
the view forms copy an exact byte range and therefore preserve embedded NUL
bytes. Assignment reuses existing capacity when possible.

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

The owned-string lifecycle is:

- `LOR_STRING_INIT`: empty owner with no allocation.
- `lor_string_assign`: create or replace the value.
- `lor_string_append`: create or extend the value.
- `lor_string_clear`: remove contents while retaining capacity.
- `lor_string_deinit`: release capacity and return to the initializer state.

When `LOR_HAS_CLEANUP_ATTRIBUTE` is nonzero, `LOR_AUTO_STRING` calls
`lor_string_deinit` automatically at scope exit:

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
