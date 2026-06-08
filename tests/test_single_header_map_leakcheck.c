// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MAP
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

typedef MAP_ENTRY(StringView, int) WordEntry;

int main(void) {
    size_t before = leakcheck_count();
    {
        AUTO_MAP WordEntry *map = MAP_INIT;
        CHECK(map_init(
                  map, map_config_string_view(MAP_KEY_OWNED)) ==
              STATUS_OK);
        CHECK(map_put_as(map, WordEntry, sv_from_cstr("owned"), 1) ==
              STATUS_OK);
    }
    CHECK(leakcheck_count() == before);

    puts("test_single_header_map_leakcheck: ok");
    return 0;
}
