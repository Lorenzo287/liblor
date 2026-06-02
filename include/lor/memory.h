// SPDX-License-Identifier: MIT

#ifndef LOR_MEMORY_H
#define LOR_MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Realloc-style allocator callback.
   new_size == 0 frees ptr and returns NULL. On allocation failure, return NULL
   and leave ptr valid. Alignment is normal malloc-style alignment. */
typedef void *(*LorAllocatorReallocFn)(void *ctx, void *ptr, size_t old_size,
                                       size_t new_size);

typedef struct LorAllocator {
    void *ctx;
    LorAllocatorReallocFn realloc;
} LorAllocator;

#define LOR_ALLOCATOR_INIT {NULL, NULL}

LorAllocator lor_allocator_heap(void);
int lor_allocator_is_valid(LorAllocator allocator);

/* Convenience wrappers. Passing an invalid allocator fails allocations and makes
   frees no-ops. Array helpers return NULL on zero counts or overflow. */
void *lor_allocator_realloc(LorAllocator allocator, void *ptr, size_t old_size,
                            size_t new_size);
void *lor_allocator_alloc(LorAllocator allocator, size_t size);
void *lor_allocator_alloc_zero(LorAllocator allocator, size_t size);
void lor_allocator_free(LorAllocator allocator, void *ptr, size_t old_size);

void *lor_allocator_alloc_array(LorAllocator allocator, size_t count,
                                size_t elem_size);
void *lor_allocator_alloc_array_zero(LorAllocator allocator, size_t count,
                                     size_t elem_size);
void *lor_allocator_realloc_array(LorAllocator allocator, void *ptr,
                                  size_t old_count, size_t new_count,
                                  size_t elem_size);

typedef struct LorArenaBlock LorArenaBlock;

typedef struct LorArena {
    LorArenaBlock *blocks;
    size_t block_size;
} LorArena;

#define LOR_ARENA_DEFAULT_BLOCK_SIZE ((size_t)64 * 1024u)
#define LOR_ARENA_INIT {NULL, 0u}

void lor_arena_init(LorArena *arena, size_t block_size);
void lor_arena_deinit(LorArena *arena);
void lor_arena_reset(LorArena *arena);

void *lor_arena_alloc(LorArena *arena, size_t size);
void *lor_arena_alloc_zero(LorArena *arena, size_t size);
void *lor_arena_alloc_aligned(LorArena *arena, size_t size, size_t alignment);

void *lor_arena_alloc_array(LorArena *arena, size_t count, size_t elem_size);
void *lor_arena_alloc_array_zero(LorArena *arena, size_t count, size_t elem_size);

char *lor_arena_strdup(LorArena *arena, const char *text);

size_t lor_arena_used(const LorArena *arena);
size_t lor_arena_capacity(const LorArena *arena);

#ifdef __cplusplus
}
#endif

#endif
