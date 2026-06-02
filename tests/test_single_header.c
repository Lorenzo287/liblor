// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_LEAKCHECK_STDLIB
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
    Arena arena = ARENA_INIT;
    int *values = (int *)arena_alloc_array_zero(&arena, 4, sizeof(*values));
    ArenaTemp temp = arena_temp_begin(&arena);
    char *scratch = (char *)arena_alloc(&arena, 32);
    char *heap = NULL;

    CHECK(values != NULL);
    CHECK(arena_used(&arena) >= 4 * sizeof(*values));
    CHECK(scratch != NULL);

    arena_temp_end(temp);
    arena_deinit(&arena);

    leakcheck_enable(1);
    heap = (char *)malloc(16);
    CHECK(heap != NULL);
    CHECK(leakcheck_count() == 1);
    free(heap);
    CHECK(leakcheck_count() == 0);
    leakcheck_enable(0);

    puts("test_single_header: ok");
    return 0;
}
