// SPDX-License-Identifier: MIT

#ifndef LOR_ARENA_H
#define LOR_ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
