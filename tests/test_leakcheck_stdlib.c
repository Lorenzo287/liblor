// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>

#define LOR_LEAKCHECK_STDLIB
#include "lor/memory.h"

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void) {
    char *text = NULL;
    int *values = NULL;

    lor_leakcheck_enable(1);
    text = strdup("stdlib");
    CHECK(text != NULL);
    values = (int *)calloc(4, sizeof(*values));
    CHECK(values != NULL);
    CHECK(lor_leakcheck_count() == 2);

    text = (char *)realloc(text, 64);
    CHECK(text != NULL);
    CHECK(lor_leakcheck_count() == 2);

    free(values);
    free(text);
    CHECK(lor_leakcheck_count() == 0);
    lor_leakcheck_enable(0);

    puts("test_leakcheck_stdlib: ok");
    return 0;
}
