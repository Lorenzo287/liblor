// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_STRING
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    CHECK(leakcheck_count() == 0);
    {
        AUTO_STRING String string = STRING_INIT;
        CHECK(string_append_cstr(&string, "tracked string") == STATUS_OK);
        CHECK(leakcheck_count() == 1);
    }
    CHECK(leakcheck_count() == 0);

    puts("test_single_header_string_leakcheck: ok");
    return 0;
}
