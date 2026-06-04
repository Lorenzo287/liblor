#include "lor/memory.h"
#include <stdio.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;
    int *permanent = (int *)lor_arena_alloc(&arena, sizeof(*permanent));
    if (permanent == NULL) {
        lor_arena_deinit(&arena);
        return 1;
    }

    size_t before = 0;
    size_t during = 0;
    size_t after = 0;

    *permanent = 42;
    before = lor_arena_used(&arena);

    LorArenaMark mark = lor_arena_mark(&arena);

    char *message = lor_arena_strdup(&arena, "temporary arena text");
    if (message == NULL) {
        lor_arena_rewind(&arena, mark);
        lor_arena_deinit(&arena);
        return 1;
    }

    during = lor_arena_used(&arena);
    printf("%s\n", message);

    lor_arena_rewind(&arena, mark);
    after = lor_arena_used(&arena);

    printf("permanent=%d before=%zu during=%zu after=%zu\n", *permanent, before,
           during, after);

    lor_arena_deinit(&arena);
    return 0;
}
