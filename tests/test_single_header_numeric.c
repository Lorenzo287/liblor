// SPDX-License-Identifier: MIT

#define LOR_ENABLE_NUMERIC
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    int a = 7;
    int b = 3;

    CHECK(lor_min(a++, b++) == 3);
    CHECK(a == 8);
    CHECK(b == 4);
    CHECK(lor_max(4, 9) == 9);
    CHECK(lor_clamp(12, 0, 10) == 10);

    puts("test_single_header_numeric: ok");
    return 0;
}
