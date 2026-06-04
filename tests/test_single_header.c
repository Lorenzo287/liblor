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
    Arena configured = ARENA_INIT;
    ArenaMark mark = ARENA_MARK_INIT;
    int *values = (int *)arena_alloc_array(&arena, 4, sizeof(*values),
                                           .zero = true);
    int *configured_value = NULL;
    char *scratch = NULL;
    char *heap = NULL;

    CHECK(values != NULL);
    CHECK(arena_used(&arena) >= 4 * sizeof(*values));
    mark = arena_mark(&arena);
    scratch = (char *)arena_alloc(&arena, 32);
    CHECK(scratch != NULL);

    arena_rewind(&arena, mark);
    arena_deinit(&arena);

    CHECK(arena_init(&configured, .block_size = 128));
    configured_value = (int *)arena_alloc(&configured, sizeof(*configured_value),
                                          .zero = true);
    CHECK(configured_value != NULL);
    CHECK(*configured_value == 0);
    arena_deinit(&configured);

    heap = (char *)malloc(16);
    CHECK(heap != NULL);
    CHECK(leakcheck_count() == 1);
    free(heap);
    CHECK(leakcheck_count() == 0);

    puts("test_single_header: ok");
    return 0;
}
