// SPDX-License-Identifier: MIT

#ifndef LOR_NUMERIC_H
#define LOR_NUMERIC_H

#include "lor/features.h"

/* Numeric minimum, maximum, and clamp helpers.

   The inferred forms use the common arithmetic type of their arguments and
   evaluate each argument exactly once. GCC and Clang use `lor_typeof` plus a
   statement expression. Other C11 compilers use `_Generic` dispatch to inline
   functions with the same single-evaluation guarantee.

   The explicit `_as` forms are the standard C11 alternative when callers want
   to choose the result type. C++ translation units do not expose these C
   convenience macros. Floating-point helpers use ordinary comparison
   semantics rather than the special NaN handling of `fmin` and `fmax`. */

#if defined(__GNUC__) || defined(__clang__)
#define LOR_NUMERIC__MAYBE_UNUSED __attribute__((unused))
#else
#define LOR_NUMERIC__MAYBE_UNUSED
#endif

#define LOR_NUMERIC__DEFINE(type, suffix)                                 \
    static inline type LOR_NUMERIC__MAYBE_UNUSED                          \
        lor_numeric__min_##suffix(type a, type b) {                        \
        return a < b ? a : b;                                              \
    }                                                                      \
    static inline type LOR_NUMERIC__MAYBE_UNUSED                           \
        lor_numeric__max_##suffix(type a, type b) {                        \
        return a > b ? a : b;                                              \
    }                                                                      \
    static inline type LOR_NUMERIC__MAYBE_UNUSED                           \
        lor_numeric__clamp_##suffix(type value, type lower, type upper) {  \
        return value < lower ? lower : value > upper ? upper : value;      \
    }

LOR_NUMERIC__DEFINE(char, char)
LOR_NUMERIC__DEFINE(signed char, signed_char)
LOR_NUMERIC__DEFINE(unsigned char, unsigned_char)
LOR_NUMERIC__DEFINE(short, short)
LOR_NUMERIC__DEFINE(unsigned short, unsigned_short)
LOR_NUMERIC__DEFINE(int, int)
LOR_NUMERIC__DEFINE(unsigned int, unsigned_int)
LOR_NUMERIC__DEFINE(long, long)
LOR_NUMERIC__DEFINE(unsigned long, unsigned_long)
LOR_NUMERIC__DEFINE(long long, long_long)
LOR_NUMERIC__DEFINE(unsigned long long, unsigned_long_long)
LOR_NUMERIC__DEFINE(float, float)
LOR_NUMERIC__DEFINE(double, double)
LOR_NUMERIC__DEFINE(long double, long_double)

#undef LOR_NUMERIC__DEFINE
#undef LOR_NUMERIC__MAYBE_UNUSED

#if LOR_HAS_GENERIC_SELECTION
#define LOR_HAS_NUMERIC_AS 1
#define LOR_NUMERIC__SELECT(value, operation)                             \
    _Generic((value),                                                     \
        char: lor_numeric__##operation##_char,                            \
        signed char: lor_numeric__##operation##_signed_char,              \
        unsigned char: lor_numeric__##operation##_unsigned_char,          \
        short: lor_numeric__##operation##_short,                          \
        unsigned short: lor_numeric__##operation##_unsigned_short,        \
        int: lor_numeric__##operation##_int,                              \
        unsigned int: lor_numeric__##operation##_unsigned_int,            \
        long: lor_numeric__##operation##_long,                            \
        unsigned long: lor_numeric__##operation##_unsigned_long,          \
        long long: lor_numeric__##operation##_long_long,                  \
        unsigned long long: lor_numeric__##operation##_unsigned_long_long, \
        float: lor_numeric__##operation##_float,                          \
        double: lor_numeric__##operation##_double,                        \
        long double: lor_numeric__##operation##_long_double)

#define lor_min_as(type, a, b) \
    LOR_NUMERIC__SELECT((type){0}, min)((type)(a), (type)(b))
#define lor_max_as(type, a, b) \
    LOR_NUMERIC__SELECT((type){0}, max)((type)(a), (type)(b))
#define lor_clamp_as(type, value, lower, upper)               \
    LOR_NUMERIC__SELECT((type){0}, clamp)(                    \
        (type)(value), (type)(lower), (type)(upper))
#else
#define LOR_HAS_NUMERIC_AS 0
#endif

#if LOR_HAS_TYPEOF && LOR_HAS_STATEMENT_EXPRESSIONS
#define LOR_HAS_NUMERIC_AUTO 1
#define lor_min(a, b)                                                       \
    __extension__({                                                         \
        lor_typeof((a) + (b)) lor_numeric__min_a = (a);                     \
        lor_typeof((a) + (b)) lor_numeric__min_b = (b);                     \
        lor_numeric__min_a < lor_numeric__min_b ? lor_numeric__min_a        \
                                                 : lor_numeric__min_b;       \
    })
#define lor_max(a, b)                                                       \
    __extension__({                                                         \
        lor_typeof((a) + (b)) lor_numeric__max_a = (a);                     \
        lor_typeof((a) + (b)) lor_numeric__max_b = (b);                     \
        lor_numeric__max_a > lor_numeric__max_b ? lor_numeric__max_a        \
                                                 : lor_numeric__max_b;       \
    })
#define lor_clamp(value, lower, upper)                                      \
    __extension__({                                                         \
        lor_typeof((value) + (lower) + (upper)) lor_numeric__clamp_value =  \
            (value);                                                        \
        lor_typeof((value) + (lower) + (upper)) lor_numeric__clamp_lower =  \
            (lower);                                                        \
        lor_typeof((value) + (lower) + (upper)) lor_numeric__clamp_upper =  \
            (upper);                                                        \
        lor_numeric__clamp_value < lor_numeric__clamp_lower                 \
            ? lor_numeric__clamp_lower                                      \
            : lor_numeric__clamp_value > lor_numeric__clamp_upper           \
                  ? lor_numeric__clamp_upper                                \
                  : lor_numeric__clamp_value;                               \
    })
#elif LOR_HAS_GENERIC_SELECTION
#define LOR_HAS_NUMERIC_AUTO 1
#define lor_min(a, b) LOR_NUMERIC__SELECT((a) + (b), min)((a), (b))
#define lor_max(a, b) LOR_NUMERIC__SELECT((a) + (b), max)((a), (b))
#define lor_clamp(value, lower, upper)                                     \
    LOR_NUMERIC__SELECT((value) + (lower) + (upper), clamp)(               \
        (value), (lower), (upper))
#else
#define LOR_HAS_NUMERIC_AUTO 0
#endif

#endif
