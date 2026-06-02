// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_ARENA
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#define CHECK(expr) do {                  \
    if (!(expr)) {                        \
        fprintf(stderr, "check failed\n"); \
        return 1;                         \
    }                                     \
} while (0)

int main(void) {
    Arena arena = ARENA_INIT;
    int *values = (int *)arena_alloc_array_zero(&arena, 4, sizeof(*values));

    CHECK(values != NULL);
    CHECK(arena_used(&arena) >= 4 * sizeof(*values));

    arena_deinit(&arena);
    puts("test_single_header: ok");
    return 0;
}
