// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    CHECK(!leakcheck_is_enabled());
    CHECK(leakcheck_count() == 0);
    CHECK(leakcheck_report(NULL) == 0);

    puts("test_single_header_memory: ok");
    return 0;
}
