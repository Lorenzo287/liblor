// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#define CHECK(expr)                            \
    do {                                       \
        if (!(expr)) {                         \
            fprintf(stderr, "check failed\n"); \
            return 1;                          \
        }                                      \
    } while (0)

int main(void) {
    Allocator allocator = allocator_heap();
    Arena arena = ARENA_INIT;
    int *heap_values =
        (int *)allocator_alloc_array_zero(allocator, 4, sizeof(*heap_values));
    int *values = (int *)arena_alloc_array_zero(&arena, 4, sizeof(*values));

    CHECK(heap_values != NULL);
    CHECK(heap_values[0] == 0);
    CHECK(values != NULL);
    CHECK(arena_used(&arena) >= 4 * sizeof(*values));

    allocator_free(allocator, heap_values, 4 * sizeof(*heap_values));
    arena_deinit(&arena);
    puts("test_single_header: ok");
    return 0;
}
