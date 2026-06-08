// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_SET
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    size_t before = leakcheck_count();
    {
        AUTO_SET StringView *words = SET_INIT;
        CHECK(set_init(
                  words, set_config_string_view(SET_KEY_OWNED)) ==
              STATUS_OK);
        StringView owned = sv_from_cstr("owned");
        CHECK(set_add(words, owned) == STATUS_OK);
    }
    CHECK(leakcheck_count() == before);

    puts("test_single_header_set_leakcheck: ok");
    return 0;
}
