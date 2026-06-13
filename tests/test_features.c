// SPDX-License-Identifier: MIT

#include <stdio.h>

#include "lor/features.h"

#if !LOR_HAS_GENERIC_SELECTION
#error "The project C compilers must support generic selection"
#endif

#if !LOR_HAS_COMPOUND_LITERALS
#error "The project C compilers must support compound literals"
#endif

#if !LOR_HAS_TYPEOF
#error "The project C compilers must support typeof"
#endif

#if !LOR_HAS_STATEMENT_EXPRESSIONS
#error "The project C compilers must support statement expressions"
#endif

#define TEST_FEATURES_KIND(value) \
    _Generic((value), int: 1, LOR_GENERIC_LONG_DOUBLE_CASE(3) double: 2, default: 0)

#define TEST_FEATURES_INCREMENT(value) \
    __extension__({                    \
        lor_typeof(value) temporary = (value); \
        temporary + 1;                 \
    })

int main(void) {
    int value = 41;
    lor_typeof(value) copy = value;

    if (TEST_FEATURES_KIND(copy) != 1)
        return 1;
    if (TEST_FEATURES_KIND(3.5) != 2)
        return 1;
#if (defined(_MSC_VER) && !defined(__clang__)) || \
    (defined(__TINYC__) && defined(_WIN32))
    if (TEST_FEATURES_KIND(3.5L) != 2)
        return 1;
#else
    if (TEST_FEATURES_KIND(3.5L) != 3)
        return 1;
#endif
    if (TEST_FEATURES_INCREMENT(copy) != 42)
        return 1;

    puts("test_features: ok");
    return 0;
}
