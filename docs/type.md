# Type Helpers

`lor/type.h` provides two related facilities:

- standard C11 generic inspection for common built-in value types;
- a feature-gated declaration helper around compiler or C23 `typeof`.

## Type Names

`lor_type_kind` returns a `LorTypeKind`, while `lor_type_name` returns its stable
display name:

```c
printf("%s\n", lor_type_name(42));       // int
printf("%s\n", lor_type_name(3.14));     // double
printf("%s\n", lor_type_name("liblor")); // C string
```

`lor_type_kind_name(kind)` returns a stable display name for a `LorTypeKind`.

Both are macros built on standard C11 `_Generic`. Their controlling expression
is not evaluated:

```c
int value = 10;
printf("%s\n", lor_type_name(value++));
// value is still 10
```

The mapping covers booleans, character and integer types, floating types, C
strings, `void *` variants, and concrete liblor values:

- `LorStringView`;
- `LorArenaConfig`, `LorArena`, `LorArenaMark`, and `LorScratch`;
- `LorMmap` and `LorLeakStats`;
- `LorRandom`.

`LorString` remains a C string because it is deliberately a `char *` typedef.
Arrays, maps, and sets remain `LOR_TYPE_OTHER` because their handles are
ordinary typed pointers. For example, C cannot distinguish an `int *` dynamic
array from an `int *` set or an unrelated `int *`. Generic printing solves that
ambiguity with explicit `lor_print_array`, `lor_print_map`, and `lor_print_set`
wrappers.

Other typedefs map to their compatible C type. Unlisted structures, unions,
arrays that do not decay, and typed pointers return `LOR_TYPE_OTHER`.

`LOR_HAS_GENERIC_SELECTION` is nonzero when the convenience macros are
available. This includes C11-or-newer GCC, Clang, and MSVC modes. TCC is
detected explicitly because it supports `_Generic` while reporting C99 through
`__STDC_VERSION__`. C++ code can always use `LorTypeKind` and
`lor_type_kind_name` directly.

Native MSVC and Windows TCC represent `long double` as `double`. On those
targets, generic type inspection consequently reports `LOR_TYPE_DOUBLE` for
both types; listing separate associations would violate `_Generic`'s
compatible-type rules.

Project-specific type names remain straightforward without making liblor own a
global type registry:

```c
#define app_type_name(value) \
    _Generic((value), Point: "Point", default: lor_type_name(value))
```

## Declaration Type Inference

`lor_typeof(expression)` can declare an object with the type of another
expression:

```c
Point point = {3, 4};

#if LOR_HAS_TYPEOF
lor_typeof(point) copy = point;
#endif
```

GCC and Clang use `__typeof__`. MSVC uses `__typeof__` when version 19.39 or
newer is available. TCC uses its GNU-compatible `typeof`, while C23
implementations may provide standard `typeof`. Check `LOR_HAS_TYPEOF`; portable
APIs must retain an explicit-type alternative.

The capability macros and `lor_typeof` declaration helper are declared by
`lor/features.h`, which can be included without the richer type-name module.
`LOR_HAS_STATEMENT_EXPRESSIONS` is separate because inferred container helpers
need both features. GCC, Clang, and TCC provide statement expressions; MSVC
does not. Therefore modern MSVC can use `lor_typeof` and helpers that require
only type inference, but not the current array, map, set, or numeric automatic
macros built with statement expressions. Their `_as` and lvalue forms remain
the portable alternatives.
