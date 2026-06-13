# Dynamic Arrays

liblor dynamic arrays are typed pointers with private size, capacity, and
element-size metadata stored immediately before the first element.

```c
int *numbers = LOR_ARRAY_INIT;

lor_array_push_as(numbers, int, 10);
lor_array_push_as(numbers, int, 20);

for (size_t i = 0; i < lor_array_size(numbers); ++i)
    printf("%d\n", numbers[i]);

lor_array_deinit(&numbers);
```

`lor_array_capacity(array)` returns the total allocated capacity, and
`lor_array_element_size(array)` returns the stored element size.
`lor_array_clear(array)` removes all elements while retaining the allocation.

The handle supports normal indexing and pointer iteration. It must begin as
`NULL`, may move after an allocating operation, and must be released with
`lor_array_deinit`, not `free`.

## Typed Operations

The following function-like macros infer the element size from `sizeof *(array)`
and forward to checked implementation functions. The macros exist only to
simplify the API syntax:

- `lor_array_reserve(array, capacity)`
- `lor_array_resize(array, size)`
- `lor_array_append(array, elements, count)`
- `lor_array_append_array(array, source)`
- `lor_array_push(array, value)`
- `lor_array_push_as(array, type, ...)`
- `lor_array_push_auto(array, value)` when `typeof` and statement expressions
  are available, including GCC, Clang, and TCC
- `lor_array_insert_many(array, index, elements, count)`
- `lor_array_insert(array, index, value)`
- `lor_array_insert_as(array, index, type, ...)`
- `lor_array_shrink_to_fit(array)`

## Push Forms

There are three push forms with different portability and convenience
tradeoffs.

All three eventually call `lor_array_append_raw`, which in this case copies
one element (the same function is used by `lor_array_append` ecc to copy many) 
from a source address to the destination array simply by knowing its size:

```c
lor_array_append_raw(&array, sizeof *array, source_pointer, 1);
```

The function backend knows only the element size and source bytes. It cannot
receive an arbitrary C expression, infer its type, or perform a conversion by
itself. A push wrapper must therefore provide the address of an object whose
stored representation matches the array element type.

`lor_array_push` is fully portable and accepts an lvalue: a named object or a
compound literal whose address C can take. The value must have the array's
exact element type because this form copies its bytes without an implicit
conversion:

```c
int value = 10;
lor_array_push(numbers, value);
lor_array_push(points, ((Point){.x = 1, .y = 2}));

// this is not allowed!
lor_array_push(numbers, 10);
```

The literal `10` is an rvalue rather than an object with an address, so the
macro expansion `&(10)` is invalid C. Similarly, passing an lvalue of a
different type is unsafe even when C would normally convert it during
assignment:

```c
short value = 10;
lor_array_push(numbers, value); // Wrong when numbers is int *.
```

No assignment occurs here; the backend would attempt to copy `sizeof(int)`
bytes starting at a `short`.

An extra pair of parentheses is needed around a compound literal containing
commas because it is passed as one macro argument.

`lor_array_push_as` is the convenient portable C99 form for literals and inline
aggregate initialization. It requires the element type explicitly, but its
variadic initializer naturally accepts commas:

```c
lor_array_push_as(numbers, int, 20);
lor_array_push_as(points, Point, .x = 3, .y = 4);
```

This form creates an addressable compound literal of the named type and passes
its address to the same backend.

`lor_array_push_auto` is available when `LOR_HAS_ARRAY_PUSH_AUTO` is nonzero.
It uses liblor's feature-gated `lor_typeof` and statement-expression helpers to
infer the destination type, create a temporary, and evaluate the supplied
expression once:

```c
#if LOR_HAS_ARRAY_PUSH_AUTO
lor_array_push_auto(numbers, 30);

Point point = {.x = 5, .y = 6};
lor_array_push_auto(points, point);
lor_array_push_auto(points, ((Point){.x = 7, .y = 8}));
#endif
```

The automatic form works with user-defined structures because its temporary
has the type of `*array`. Scalar expressions undergo normal assignment
conversion to that type. Prefer it when a project already targets GCC or Clang
and concise literal pushes matter. Prefer `lor_array_push_as` for portable
inline construction, especially structures, and `lor_array_push` when the
value already exists as an object.

Allocating operations return `LorStatus`, preserve the original array on
failure, and reject element-size mismatches. `lor_array_resize` zero-initializes
new elements. `lor_array_clear` and shrinking retain capacity.

`lor_array_last(array)` returns a typed pointer to the final element or `NULL`.
`lor_array_pop`, `lor_array_remove`, `lor_array_remove_range`, and
`lor_array_remove_unordered` return zero when no element can be removed.
Ordered removal shifts later elements; unordered removal replaces the removed
element with the final element.

Insertion accepts any index through the current size and supports source
elements from the same array, even when the source spans the insertion point.
`lor_array_append_array` validates that two dynamic arrays use the same element
size. `lor_array_shrink_to_fit` reduces capacity to the current size and
releases an empty allocation completely.

## Ownership And Lifetime

Pointers to elements become invalid after reserve, resize, append, or push when
those operations grow the allocation. Appending elements from the same array is
supported, including when growth reallocates it.

On GCC and Clang, `LOR_AUTO_ARRAY` calls `lor_array_deinit` at scope exit:

```c
LOR_AUTO_ARRAY int *numbers = LOR_ARRAY_INIT;
```

On other compilers the macro is empty, so explicit deinitialization remains
required. When `LOR_LEAKCHECK` is enabled, the complete prefix-header
allocation is tracked as one heap allocation.
