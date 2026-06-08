// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_SET
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    int *a = SET_INIT;
    int *b = SET_INIT;
#if HAS_SET_AUTO
    CHECK(set_add_auto(a, 1) == STATUS_OK);
#else
    CHECK(set_add_as(a, int, 1) == STATUS_OK);
#endif
    CHECK(set_add_as(a, int, 2) == STATUS_OK);
    CHECK(set_add_as(b, int, 2) == STATUS_OK);
    CHECK(set_add_as(b, int, 3) == STATUS_OK);

    int *combined = SET_INIT;
    CHECK(set_union(combined, a, b) == STATUS_OK);
    CHECK(set_size(combined) == 3);
    CHECK(set_contains_as(combined, int, 1));
    CHECK(set_contains_as(combined, int, 2));
    CHECK(set_contains_as(combined, int, 3));

    set_deinit(&combined);
    set_deinit(&b);
    set_deinit(&a);
    puts("test_single_header_set: ok");
    return 0;
}
