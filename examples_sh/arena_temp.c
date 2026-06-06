#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_STRIP_PREFIX
#include "../lor.h"

int main(void) {
    Arena arena = ARENA_INIT;

    printf("==========================================\n");
    printf("  PART 1: ARENA RESET (HIGH THROUGHPUT)\n");
    printf("==========================================\n");
    
    // Allocate some memory
    arena_alloc(&arena, 1024);
    printf("After allocation: used=%zu, capacity=%zu\n", 
           arena_used(&arena), arena_capacity(&arena));

    // Resetting the arena keeps the capacity but sets "used" back to 0.
    // This avoids giving memory back to the OS, making it perfect for 
    // loops where you need fresh memory every frame or request.
    arena_reset(&arena);
    printf("After reset     : used=%zu, capacity=%zu  <-- Capacity retained!\n", 
           arena_used(&arena), arena_capacity(&arena));


    printf("\n==========================================\n");
    printf("  PART 2: MARKS AND REWIND (CHECKPOINTS)\n");
    printf("==========================================\n");
    
    // Allocate some permanent data that we want to keep around.
    arena_alloc(&arena, 500);
    printf("Permanent data used : %zu\n", arena_used(&arena));

    // Create a mark (a checkpoint of the current arena state).
    ArenaMark mark = arena_mark(&arena);

    // Do some temporary work (like string building or parsing).
    arena_alloc(&arena, 2000);
    printf("After temp work used: %zu\n", arena_used(&arena));

    // Rewind back to the mark. This instantly discards the temporary work 
    // but leaves the permanent data perfectly intact.
    arena_rewind(&arena, mark);
    printf("After rewind used   : %zu  <-- Temp data instantly discarded!\n", 
           arena_used(&arena));

    // Finally, deinit gives everything back to the OS.
    arena_deinit(&arena);
    return 0;
}
