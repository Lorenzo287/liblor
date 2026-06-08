// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MAP
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

typedef MAP_ENTRY(int, int) IntEntry;

int main(void) {
    IntEntry *map = MAP_INIT;
#if HAS_MAP_AUTO
    CHECK(map_put_auto(map, 1, 10) == STATUS_OK);
#else
    CHECK(map_put_as(map, IntEntry, 1, 10) == STATUS_OK);
#endif
    CHECK(map_put_as(map, IntEntry, 2, 20) == STATUS_OK);
    int key = 2;
    IntEntry *entry = map_find(map, key);
    CHECK(entry != NULL && entry->value == 20);
    CHECK(map_remove(map, key));
    CHECK(map_size(map) == 1);
    map_deinit(&map);

    puts("test_single_header_map: ok");
    return 0;
}
