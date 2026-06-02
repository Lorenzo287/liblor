// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_CUSTOM_PREFIX my_
#include "../lor.h"

#include <stdio.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;
    int *value = NULL;
    char *copy = NULL;

    value = (int *)my_arena_alloc(&arena, sizeof(*value));
    if (value == NULL) { return 1; }
    *value = 42;

    copy = my_arena_strdup(&arena, "custom prefix");
    if (copy == NULL) {
        my_arena_deinit(&arena);
        return 1;
    }

    my_arena_deinit(&arena);
    puts("test_single_header_custom_prefix: ok");
    return 0;
}
