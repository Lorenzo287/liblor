// SPDX-License-Identifier: MIT

#include "lor/lor.h"

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

	// NOTE: begin
    LorArenaTemp temp = lor_arena_temp_begin(&arena);
    char *message = lor_arena_strdup(&arena, "temporary arena text");
    if (message == NULL) {
        lor_arena_temp_end(temp);
        lor_arena_deinit(&arena);
        return 1;
    }

    during = lor_arena_used(&arena);
    printf("%s\n", message);

	// NOTE: end
    lor_arena_temp_end(temp);
    after = lor_arena_used(&arena);

    printf("permanent=%d before=%zu during=%zu after=%zu\n", *permanent, before,
           during, after);

    lor_arena_deinit(&arena);
    return 0;
}
