// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_ARRAY
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    CHECK(leakcheck_count() == 0);
    {
        AUTO_ARRAY int *numbers = ARRAY_INIT;
        CHECK(array_push_as(numbers, int, 42) == STATUS_OK);
        CHECK(leakcheck_count() == 1);
    }
    CHECK(leakcheck_count() == 0);

    puts("test_single_header_array_leakcheck: ok");
    return 0;
}
