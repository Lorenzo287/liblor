// SPDX-License-Identifier: MIT

#define LOR_LEAKCHECK_STDLIB
#include "lor/lor.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;
    LorVirtualMemory memory = {0};
    char *heap = NULL;

    lor_leakcheck_enable(1);

    heap = strdup("tracked by stdlib macro mode");
    lor_arena_init(&arena, 0);
    memory = lor_virtual_alloc(lor_page_size() * 2u, lor_page_size());

    if (heap == NULL || memory.ptr == NULL || lor_arena_alloc(&arena, 64) == NULL) {
        free(heap);
        lor_virtual_release(&memory);
        lor_arena_deinit(&arena);
        return 1;
    }

    printf("tracked allocations before cleanup: %zu\n", lor_leakcheck_count());
    (void)lor_leakcheck_report(stdout);

    free(heap);
    lor_virtual_release(&memory);
    lor_arena_deinit(&arena);

    printf("tracked allocations after cleanup: %zu\n", lor_leakcheck_count());
    lor_leakcheck_enable(0);
    return 0;
}
