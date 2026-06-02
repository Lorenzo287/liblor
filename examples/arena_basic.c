// SPDX-License-Identifier: MIT

#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

int main(void) {
    Arena arena = ARENA_INIT;
    int *values = arena_alloc_array_zero(&arena, 4, sizeof(*values));
    char *label = arena_strdup(&arena, "arena example");

    if (values == NULL || label == NULL) {
        arena_deinit(&arena);
        return 1;
    }

    for (size_t i = 0; i < 4; ++i) { values[i] = (int)(i + 1); }

    printf("%s: %d %d %d %d\n", label, values[0], values[1], values[2], values[3]);
    arena_deinit(&arena);
    return 0;
}
