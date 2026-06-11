#include "lor/memory.h"
#include <stdio.h>

int main(void) {
    LorScratch scratch1 = lor_scratch_begin(NULL);
    if (scratch1.arena == NULL) return 1;

    char *a = lor_arena_strdup(scratch1.arena, "first scratch arena");
    if (a == NULL) {
        lor_scratch_end(scratch1);
        return 1;
    }

    // Avoid reusing the arena whose allocations are still live.
    LorScratch scratch2 = lor_scratch_begin(scratch1.arena);
    if (scratch2.arena == NULL) {
        lor_scratch_end(scratch1);
        return 1;
    }

    char *b = lor_arena_strdup(scratch2.arena, "second scratch arena");
    if (b == NULL) {
        lor_scratch_end(scratch2);
        lor_scratch_end(scratch1);
        return 1;
    }

    printf("scratch1: %s\nscratch2: %s\n", a, b);

    lor_scratch_end(scratch2);
    lor_scratch_end(scratch1);
    lor_scratch_cleanup();
    return 0;
}
