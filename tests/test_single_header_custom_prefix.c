// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_CUSTOM_PREFIX my_
#include "../lor.h"

#include <stdio.h>

int main(void) {
    LorAllocator allocator = my_allocator_heap();
    LorArena arena = LOR_ARENA_INIT;
    int *value = NULL;
    char *copy = NULL;

    value = (int *)my_allocator_alloc(allocator, sizeof(*value));
    if (value == NULL) { return 1; }
    *value = 42;

    my_arena_init(&arena, 0);
    copy = my_arena_strdup(&arena, "custom prefix");
    if (copy == NULL) {
        my_allocator_free(allocator, value, sizeof(*value));
        my_arena_deinit(&arena);
        return 1;
    }

    my_allocator_free(allocator, value, sizeof(*value));
    my_arena_deinit(&arena);
    puts("test_single_header_custom_prefix: ok");
    return 0;
}
