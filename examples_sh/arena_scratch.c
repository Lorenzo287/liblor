#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_STRIP_PREFIX
#include "../lor.h"

static void inner_helper(Arena *conflict_arena) {
    // The inner helper needs its own temporary memory.
    // Passing the outer arena guarantees scratch_begin returns the other arena.
    Scratch scratch = scratch_begin(conflict_arena);
    
    printf("  [Inner] Using scratch arena at: %p (Conflict was: %p)\n", 
           (void*)scratch.arena, (void*)conflict_arena);

    // Do some temporary work in the inner scratch arena without fear of
    // overwriting the outer function's data.
    arena_alloc(scratch.arena, 128);

    scratch_end(scratch);
}

static void outer_function(void) {
    // Request a thread-local scratch arena. We have no conflict initially.
    Scratch scratch = scratch_begin(NULL);

    printf("[Outer] Using scratch arena at: %p\n", (void*)scratch.arena);
    
    // We allocate some data we need to keep alive across the inner helper call.
    arena_alloc(scratch.arena, 256);

    // Passing our arena lets the helper safely preserve results allocated into it
    // when the helper ends its own scratch scope.
    inner_helper(scratch.arena);

    printf("[Outer] Inner helper finished. Our scratch arena %p is still safe!\n", 
           (void*)scratch.arena);

    scratch_end(scratch);
}

int main(void) {
    printf("\x1b[96mNESTED SCRATCH ARENAS & CONFLICTS\x1b[0m\n");
    
    outer_function();
    
    return 0;
}
