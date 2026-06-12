# Generic Printing

`lor/print.h` adds concise, type-directed printing without replacing `printf`.
Use it for ordinary value display; keep `printf` when exact field widths,
bases, precision, or format layout matter.

```c
#include "lor/print.h"

lor_print("answer", 42, 3.14);
// answer 42 3.14\n
```

Arguments are separated by one space and the default ending is a newline.
`lor_fprint` sends the same output to another `FILE *`.

## Endings

`lor_end` is a typed marker analogous to Python's `end=` option:

```c
lor_print("loading", lor_end(" ... "));
lor_print("done");
```

The marker may appear anywhere and is not treated as a printable argument, so
it never adds a separator. If several markers are supplied, the last one wins.
`lor_end("")` suppresses the newline. `lor_end_view` accepts an exact-length
`LorStringView`.

For separator control, pass a `LorPrintConfig`:

```c
LorPrintConfig csv = {LOR_SV_LITERAL(", "), LOR_SV_LITERAL("\n")};
lor_print_with(csv, "red", "green", "blue");
```

## Supported Values

The generic layer supports:

- `_Bool`, printed as `true` or `false`;
- `char`, printed as a character;
- all standard signed and unsigned integer types;
- `float`, `double`, and `long double`, normalized to `double` for portable
  display;
- mutable and constant C strings, with `NULL` displayed as `(null)`;
- `LorStringView`, written at its exact byte length;
- `void *` and `const void *`;
- concrete liblor values including arenas, mappings, leak statistics, and
  random states;
- `LorPrintValue` wrappers.

C character literals have type `int`, so use `(char)'A'` to print `A` rather
than its numeric value. In C11, the `true` macro is commonly an `int`; use a
`bool` variable or `(bool)true` when boolean spelling is desired.

`LorString` is a `char *` handle and therefore prints as a C string. Convert it
with `lor_string_view` when exact stored length or embedded NUL bytes matter.

## Containers

Arrays, maps, and sets use ordinary typed pointers. C cannot determine from an
`int *` alone whether it is a liblor array, a set, or an unrelated pointer, so
container family wrappers are required:

```c
int *numbers = LOR_ARRAY_INIT;
lor_array_push_as(numbers, int, 10);
lor_array_push_as(numbers, int, 20);

lor_print(lor_print_array(numbers)); // [10, 20]
```

Sets and maps use the same pattern:

```c
typedef LOR_MAP_ENTRY(LorStringView, int) Score;

LorStringView *tags = LOR_SET_INIT;
Score *scores = LOR_MAP_INIT;

lor_print(lor_print_set(tags)); // set()
lor_print(lor_print_map(scores)); // {}
```

Non-empty sequences use `[a, b]`, sets use `{a, b}`, and maps use
`{key: value}`. An empty set uses `set()` so it is distinct from an empty map.
C strings and string views are quoted and escaped inside containers, while
top-level strings retain their normal unquoted output. Map and set output uses
their current dense iteration order; printing does not sort or stabilize it.

`lor_print_map` uses `lor_typeof` to infer entry layout. Portable code can name
the entry type explicitly with `lor_print_map_as(map, EntryType)`;
`LOR_HAS_PRINT_MAP_AUTO` reports whether the inferred form is available.

For application-defined element types, provide a callback:

```c
Point *points = LOR_ARRAY_INIT;
lor_print(lor_print_array_custom(points, print_point));
```

The corresponding forms are `lor_print_set_custom` and
`lor_print_map_as_custom`. When `lor_typeof` is available,
`lor_print_map_custom` infers the entry type. A `NULL` callback keeps automatic
formatting for that map side, so custom keys and automatic values can be mixed.

Typed object pointers are intentionally not guessed. Wrap them explicitly:

```c
lor_print(lor_print_pointer(&object));
```

The floating conversion avoids a real Windows ABI mismatch between MinGW's
extended `long double` and the UCRT formatting implementation. Use `printf` or
a custom printer when preserving extended precision matters.

## Custom Types

User-defined values use a callback wrapper rather than modifying liblor's
generic macro:

```c
typedef struct Point {
    int x;
    int y;
} Point;

static LorStatus print_point(FILE *out, const void *value) {
    const Point *point = value;
    return fprintf(out, "Point(%d, %d)", point->x, point->y) < 0
               ? LOR_STATUS_SYSTEM_ERROR
               : LOR_STATUS_OK;
}

Point point = {3, 4};
lor_print("point", lor_print_custom(&point, print_point));
```

This keeps custom behavior local to the owning project and avoids a global
registry or an application-wide X-macro.

## Function Backend

The macro layer converts each argument to `LorPrintValue` and calls:

```c
LorStatus lor_fprint_values(FILE *out, LorPrintConfig config,
                            const LorPrintValue *values, size_t count);
```

This function is available to C++, non-generic C code, generated code, and
callers that need zero values. It returns `LOR_STATUS_INVALID_ARGUMENT` for an
invalid tagged value or view and `LOR_STATUS_SYSTEM_ERROR` for an output
failure.

`LOR_HAS_GENERIC_PRINT` indicates whether `lor_print`, `lor_fprint`, and their
`_with` forms are available. They require at least one argument and support up
to 16. Each expression is evaluated once, but C does not define the evaluation
order of initializer elements; do not make one print call depend on argument
side-effect order.
