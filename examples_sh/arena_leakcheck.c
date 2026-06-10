#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

int main(void) {
    printf("\x1b[96mSCENARIO 1: HEAP ARENA (SMALL ALLOC)\x1b[0m\n");
    Arena heap_small = ARENA_INIT;
    
    // Allocating just 4 bytes.
    // The heap arena uses a standard block size (default 64KB). 
    // It will provision a block of exactly 64KB (65536 bytes) capacity.
    // The leak checker tracks "committed" bytes, which for a heap is the full block capacity.
    arena_alloc(&heap_small, 4);
    
    printf("used=%zu, capacity=%zu, committed=%zu\n",
           arena_used(&heap_small), arena_capacity(&heap_small), arena_committed(&heap_small));
    printf("We allocated 4 bytes. The leak checker reports the full committed block:\n");
    leakcheck_report(stdout); // Should report 65536 bytes
    
    // Clean up to keep the next report clean
    arena_deinit(&heap_small);


    printf("\n\x1b[96mSCENARIO 2: HEAP ARENA (LARGE ALLOC)\x1b[0m\n");
    Arena heap_large = ARENA_INIT;

    // Allocating 400,000 bytes (larger than the 64KB default block).
    // To save memory, the heap allocator does NOT round this up to a 64KB multiple.
    // Instead, it provisions a block with a capacity of exactly 400,000 bytes.
    arena_alloc(&heap_large, 400000);
    
    printf("used=%zu, capacity=%zu, committed=%zu\n",
           arena_used(&heap_large), arena_capacity(&heap_large), arena_committed(&heap_large));
    printf("We allocated 400,000 bytes. The leak checker reports exactly that:\n");
    leakcheck_report(stdout); // Should report ~400000 bytes
    arena_deinit(&heap_large);


    printf("\n\x1b[96mSCENARIO 3: VIRTUAL ARENA (LARGE ALLOC)\x1b[0m\n");
    Arena virtual_arena = ARENA_INIT;
    
    // For virtual arenas, we MUST explicitly initialize them with a config.
    ArenaConfig v_config = {
        .backend = ARENA_BACKEND_VIRTUAL,
        // NOTE: Specifying reserve and commit sizes is optional. If left as 0, 
        // they fall back to the defaults (Reserve: 64MB, Commit: 64KB).
        // If you do specify them, you can either use multiples of lor_page_size() 
        // or any arbitrary number, which will be automatically aligned to the nearest page.
    };
    
    if (arena_init_config(&virtual_arena, v_config)) {
        // Allocating 400,000 bytes again.
        // The virtual allocator MUST commit memory in multiples of `commit_size` (64KB).
        // 400,000 bytes + metadata header, rounded up to the next 64KB boundary is
        // 458,752 bytes (which is exactly 7 * 64KB).
        // The leak checker reports committed memory MINUS the metadata header.
        arena_alloc(&virtual_arena, 400000);
        
        printf("used=%zu, capacity=%zu, committed=%zu\n",
               arena_used(&virtual_arena), arena_capacity(&virtual_arena), arena_committed(&virtual_arena));
        printf("We allocated 400,000 bytes. The virtual arena commits in 64KB chunks,\n");
        printf("so the leak checker reports the padded out size (~458,688 bytes):\n");
        leakcheck_report(stdout); 
        
        arena_deinit(&virtual_arena);
    } else {
        printf("Failed to initialize virtual arena.\n");
    }

    return 0;
}
