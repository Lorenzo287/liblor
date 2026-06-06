#include "lor/memory.h"
#include <stdio.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;

    size_t page_size = lor_page_size();
    LorArenaConfig config = {
        .backend = LOR_ARENA_BACKEND_VIRTUAL,
        .reserve_size = page_size * 8u,
        .commit_size = page_size,
    };
    if (!lor_arena_init_config(&arena, config)) return 1;

    char *buffer = (char *)lor_arena_alloc(&arena, page_size + 128u);
    if (buffer == NULL) {
        lor_arena_deinit(&arena);
        return 1;
    }

    buffer[0] = 'L';
    buffer[page_size] = 'R';

    printf("page=%zu capacity=%zu committed=%zu used=%zu first=%c later=%c\n",
           page_size, lor_arena_capacity(&arena), lor_arena_committed(&arena),
           lor_arena_used(&arena), buffer[0], buffer[page_size]);

    lor_arena_deinit(&arena);
    return 0;
}
