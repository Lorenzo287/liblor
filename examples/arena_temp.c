#include "lor/memory.h"
#include <stdio.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;
    int *permanent = (int *)lor_arena_alloc(&arena, sizeof(*permanent));
    if (permanent == NULL) {
        lor_arena_deinit(&arena);
        return 1;
    }

    *permanent = 42;
    size_t before = lor_arena_used(&arena);

    LorArenaMark mark = lor_arena_mark(&arena);

    char *message = lor_arena_strdup(&arena, "temporary arena text");
    if (message == NULL) {
        lor_arena_rewind(&arena, mark);
        lor_arena_deinit(&arena);
        return 1;
    }

    size_t during = lor_arena_used(&arena);
    printf("%s\n", message);

    lor_arena_rewind(&arena, mark);
    size_t after = lor_arena_used(&arena);

    printf("permanent=%d before=%zu during=%zu after=%zu\n", *permanent, before,
           during, after);

    lor_arena_deinit(&arena);
    return 0;
}
