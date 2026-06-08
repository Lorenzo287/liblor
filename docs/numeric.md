# Numeric Helpers

`lor/numeric.h` provides minimum, maximum, and clamp expressions for standard
arithmetic types:

```c
int low = lor_min(a, b);
double high = lor_max(2, 3.5);
size_t index = lor_clamp(position, 0u, count);
```

Each argument is evaluated exactly once. On GCC and Clang, the inferred forms
use `lor_typeof` and statement expressions. Other C11 compilers use `_Generic`
to select a typed inline function. `LOR_HAS_NUMERIC_AUTO` reports whether these
forms are available.

The common arithmetic type of the arguments becomes the result type, following
the usual C conversion rules. Use the standard C11 explicit forms when the
conversion should be chosen directly:

```c
unsigned int low = lor_min_as(unsigned int, signed_value, unsigned_value);
double unit = lor_clamp_as(double, value, 0.0, 1.0);
```

`LOR_HAS_NUMERIC_AS` reports whether the explicit forms are available.

The clamp bounds are expected to be ordered so that `lower <= upper`. Floating
helpers use ordinary `<` and `>` comparisons; use `<math.h>` functions when
specific NaN behavior is required.
