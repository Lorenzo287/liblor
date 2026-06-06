#include "lor/memory.h"
#include <stdio.h>

int main(void) {
    LorArena arena = LOR_ARENA_INIT;

    int *values = lor_arena_alloc_array_zero(&arena, 4, sizeof(*values));
    char *label = lor_arena_strdup(&arena, "arena example");
    if (values == NULL || label == NULL) {
        lor_arena_deinit(&arena);
        return 1;
    }

    for (size_t i = 0; i < 4; ++i) values[i] = (int)(i + 1);

    printf("%s: %d %d %d %d\n", label, values[0], values[1], values[2], values[3]);
    lor_arena_deinit(&arena);
    return 0;
}
