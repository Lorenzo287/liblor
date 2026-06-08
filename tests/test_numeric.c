// SPDX-License-Identifier: MIT

#include "lor/numeric.h"

#include <stddef.h>
#include <stdio.h>

#include "lor_test.h"

static int test_numeric_values(void) {
    CHECK(lor_min(4, 9) == 4);
    CHECK(lor_max(-2, -7) == -2);
    CHECK(lor_min(3, 2.5) == 2.5);
    CHECK(lor_max(3u, 8u) == 8u);
    CHECK(lor_clamp(5, 0, 10) == 5);
    CHECK(lor_clamp(-2, 0, 10) == 0);
    CHECK(lor_clamp(12, 0, 10) == 10);

    size_t size = 12u;
    CHECK(lor_min_as(size_t, size, 5u) == 5u);
    CHECK(lor_max_as(double, 4, 6.5) == 6.5);
    CHECK(lor_clamp_as(unsigned int, 20, 2, 8) == 8u);
    return 0;
}

static int test_numeric_arguments_are_evaluated_once(void) {
    int a = 2;
    int b = 5;
    CHECK(lor_min(a++, b++) == 2);
    CHECK(a == 3);
    CHECK(b == 6);

    int value = 20;
    int lower = 4;
    int upper = 12;
    CHECK(lor_clamp(value++, lower++, upper++) == 12);
    CHECK(value == 21);
    CHECK(lower == 5);
    CHECK(upper == 13);

    CHECK(lor_max_as(int, a++, b++) == 6);
    CHECK(a == 4);
    CHECK(b == 7);
    return 0;
}

int main(void) {
    CHECK(LOR_HAS_NUMERIC_AUTO);
    CHECK(LOR_HAS_NUMERIC_AS);
    CHECK(test_numeric_values() == 0);
    CHECK(test_numeric_arguments_are_evaluated_once() == 0);

    puts("test_numeric: ok");
    return 0;
}
