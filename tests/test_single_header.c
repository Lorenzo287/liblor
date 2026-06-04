// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_LEAKCHECK
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
    char *scratch = NULL;
    char *heap = NULL;

    CHECK(values != NULL);
    CHECK(arena_used(&arena) >= 4 * sizeof(*values));
    CHECK(arena_mark(&arena));
    scratch = (char *)arena_alloc(&arena, 32);
    CHECK(scratch != NULL);

    arena_rewind(&arena);
    arena_deinit(&arena);

    heap = (char *)malloc(16);
    CHECK(heap != NULL);
    CHECK(leakcheck_count() == 1);
    free(heap);
    CHECK(leakcheck_count() == 0);

    puts("test_single_header: ok");
    return 0;
}
