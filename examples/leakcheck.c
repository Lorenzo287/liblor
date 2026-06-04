// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>

#include "lor/lor.h"

int main(void) {
    LorArena arena = LOR_ARENA_INIT;

    char *heap = (char *)malloc(32);
    lor_arena_init(&arena, 0);

    if (heap == NULL || lor_arena_alloc(&arena, 64) == NULL) {
        free(heap);
        lor_arena_deinit(&arena);
        return 1;
    }

    printf("tracked allocations before cleanup: %zu\n", lor_leakcheck_count());
    (void)lor_leakcheck_report(stdout);

    free(heap);
    lor_arena_deinit(&arena);

    printf("tracked allocations after cleanup: %zu\n", lor_leakcheck_count());
    return 0;
}
