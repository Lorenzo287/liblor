// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_ARRAY
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    int *numbers = ARRAY_INIT;
#if HAS_ARRAY_PUSH_AUTO
    CHECK(array_push_auto(numbers, 1) == STATUS_OK);
#else
    CHECK(array_push_as(numbers, int, 1) == STATUS_OK);
#endif
    CHECK(array_push_as(numbers, int, 3) == STATUS_OK);
    CHECK(array_insert_as(numbers, 1, int, 2) == STATUS_OK);
    CHECK(array_size(numbers) == 3);
    CHECK(numbers[0] == 1 && numbers[1] == 2 && numbers[2] == 3);
    CHECK(*array_last(numbers) == 3);
    CHECK(array_remove_range(numbers, 1, 1));
    CHECK(numbers[1] == 3);
    array_deinit(&numbers);

    puts("test_single_header_array: ok");
    return 0;
}
