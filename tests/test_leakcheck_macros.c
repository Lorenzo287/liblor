// SPDX-License-Identifier: MIT

#include <stdio.h>

#include "lor/memory.h"

#include "lor_test.h"

int main(void) {
#if defined(LOR_LEAKCHECK)
    char *text = strdup("stdlib");
    CHECK(text != NULL);

    int *values = (int *)calloc(4, sizeof(*values));
    CHECK(values != NULL);
    CHECK(lor_leakcheck_count() == 2);

    text = (char *)realloc(text, 64);
    CHECK(text != NULL);
    CHECK(lor_leakcheck_count() == 2);

    free(values);
    free(text);
    CHECK(lor_leakcheck_count() == 0);
#else
    CHECK(lor_leakcheck_count() == 0);
#endif

    puts("test_leakcheck_macros: ok");
    return 0;
}
