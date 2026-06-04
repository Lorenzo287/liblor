// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_CUSTOM_PREFIX my_
#define LOR_LEAKCHECK
#include "../lor.h"

#include <stdio.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;
    LorArena configured = LOR_ARENA_INIT;
    int *value = NULL;
    int *zeroed = NULL;
    int *configured_value = NULL;
    char *copy = NULL;
    char *heap = NULL;

    value = (int *)lor_arena_alloc(&arena, sizeof(*value));
    if (value == NULL) { return 1; }
    *value = 42;

    zeroed = (int *)lor_arena_alloc(&arena, sizeof(*zeroed), .zero = true);
    if (zeroed == NULL || *zeroed != 0) {
        my_arena_deinit(&arena);
        return 1;
    }

    copy = my_arena_strdup(&arena, "custom prefix");
    if (copy == NULL) {
        my_arena_deinit(&arena);
        return 1;
    }

    if (!lor_arena_init(&configured, .block_size = 128)) {
        my_arena_deinit(&arena);
        return 1;
    }

    configured_value =
        (int *)lor_arena_alloc(&configured, sizeof(*configured_value));
    if (configured_value == NULL) {
        my_arena_deinit(&configured);
        my_arena_deinit(&arena);
        return 1;
    }

    heap = (char *)malloc(16);
    if (heap == NULL || my_leakcheck_count() != 3) {
        free(heap);
        my_arena_deinit(&configured);
        my_arena_deinit(&arena);
        return 1;
    }

    free(heap);
    if (my_leakcheck_count() != 2) {
        my_arena_deinit(&configured);
        my_arena_deinit(&arena);
        return 1;
    }

    my_arena_deinit(&configured);
    my_arena_deinit(&arena);
    if (my_leakcheck_count() != 0) { return 1; }

    puts("test_single_header_custom_prefix: ok");
    return 0;
}
