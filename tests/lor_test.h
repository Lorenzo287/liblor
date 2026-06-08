// SPDX-License-Identifier: MIT

#ifndef LOR_TEST_H
#define LOR_TEST_H

#include <stdio.h>

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#endif
