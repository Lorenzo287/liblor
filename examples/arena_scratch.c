#include "lor/memory.h"
#include <stdio.h>

static char *copy_result(LorArena *result_arena, const char *source) {
    // Keep temporary work separate from the arena that owns the result.
    LorScratch temporary = lor_scratch_begin(result_arena);
    if (temporary.arena == NULL) return NULL;

    char *copy = lor_arena_strdup(temporary.arena, source);
    char *result =
        copy != NULL ? lor_arena_strdup(result_arena, copy) : NULL;

    // This must discard `copy` without discarding `result`.
    lor_scratch_end(temporary);
    return result;
}

int main(void) {
    LorScratch output = lor_scratch_begin(NULL);
    if (output.arena == NULL) return 1;

    char *result = copy_result(output.arena, "result survives inner scratch");
    if (result == NULL) {
        lor_scratch_end(output);
        return 1;
    }

    printf("%s\n", result);

    lor_scratch_end(output);
    lor_scratch_cleanup();
    return 0;
}
