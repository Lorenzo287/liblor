// SPDX-License-Identifier: MIT

#include "lor/lor.h"

#include <stdio.h>

int main(void) {
    LorArenaTemp scratch = lor_scratch_begin(NULL, 0);
    LorArena *conflicts[1];
    LorArenaTemp other;
    char *a = NULL;
    char *b = NULL;

    if (scratch.arena == NULL) { return 1; }

    a = lor_arena_strdup(scratch.arena, "first scratch arena");
    if (a == NULL) {
        lor_scratch_end(scratch);
        return 1;
    }

    conflicts[0] = scratch.arena;
    other = lor_scratch_begin(conflicts, 1);
    if (other.arena == NULL) {
        lor_scratch_end(scratch);
        return 1;
    }

    b = lor_arena_strdup(other.arena, "second scratch arena");
    if (b == NULL) {
        lor_scratch_end(other);
        lor_scratch_end(scratch);
        return 1;
    }

    printf("%s / %s\n", a, b);

    lor_scratch_end(other);
    lor_scratch_end(scratch);
    lor_scratch_cleanup_current_thread();
    return 0;
}
