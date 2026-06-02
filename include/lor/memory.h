// SPDX-License-Identifier: MIT

#ifndef LOR_MEMORY_H
#define LOR_MEMORY_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOR_KIB(n) ((size_t)(n) << 10)
#define LOR_MIB(n) ((size_t)(n) << 20)
#define LOR_GIB(n) ((size_t)(n) << 30)

#define LOR_ARENA_DEFAULT_BLOCK_SIZE LOR_KIB(64)
#define LOR_ARENA_DEFAULT_RESERVE_SIZE LOR_MIB(64)
#define LOR_ARENA_DEFAULT_COMMIT_SIZE LOR_KIB(64)

typedef enum LorArenaBackend {
    LOR_ARENA_BACKEND_HEAP = 0,
    LOR_ARENA_BACKEND_VIRTUAL = 1
} LorArenaBackend;

typedef struct LorArenaBlock LorArenaBlock;

typedef struct LorArenaConfig {
    LorArenaBackend backend;
    size_t block_size;
    size_t reserve_size;
    size_t commit_size;
} LorArenaConfig;

typedef struct LorArena {
    LorArenaBlock *blocks;
    LorArenaBackend backend;
    size_t block_size;
    size_t reserve_size;  // capacity
    size_t commit_size;   // allocation size
    int leakcheck_tracked;
} LorArena;

typedef struct LorArenaMark {
    LorArenaBlock *block;
    size_t used;
} LorArenaMark;

typedef struct LorArenaTemp {
    LorArena *arena;
    LorArenaMark mark;
} LorArenaTemp;

#define LOR_ARENA_INIT {NULL, LOR_ARENA_BACKEND_HEAP, 0u, 0u, 0u, 0}

void lor_arena_init(LorArena *arena, size_t block_size);
int lor_arena_init_ex(LorArena *arena, const LorArenaConfig *config);
void lor_arena_deinit(LorArena *arena);
void lor_arena_reset(LorArena *arena);

LorArenaMark lor_arena_mark(const LorArena *arena);
void lor_arena_rewind(LorArena *arena, LorArenaMark mark);
LorArenaTemp lor_arena_temp_begin(LorArena *arena);
void lor_arena_temp_end(LorArenaTemp temp);

void *lor_arena_alloc(LorArena *arena, size_t size);
void *lor_arena_alloc_zero(LorArena *arena, size_t size);
void *lor_arena_alloc_aligned(LorArena *arena, size_t size, size_t alignment);

void *lor_arena_alloc_array(LorArena *arena, size_t count, size_t elem_size);
void *lor_arena_alloc_array_zero(LorArena *arena, size_t count, size_t elem_size);

char *lor_arena_strdup(LorArena *arena, const char *text);

size_t lor_arena_used(const LorArena *arena);
size_t lor_arena_capacity(const LorArena *arena);
size_t lor_arena_committed(const LorArena *arena);

LorArenaTemp lor_scratch_begin(LorArena **conflicts, size_t conflict_count);
void lor_scratch_end(LorArenaTemp temp);
void lor_scratch_cleanup_current_thread(void);

typedef struct LorVirtualMemory {
    void *ptr;
    size_t reserved;
    size_t committed;
} LorVirtualMemory;

size_t lor_page_size(void);
LorVirtualMemory lor_virtual_alloc(size_t reserve_size, size_t commit_size);
int lor_virtual_commit(void *ptr, size_t size);
int lor_virtual_decommit(void *ptr, size_t size);
void lor_virtual_release(LorVirtualMemory *memory);

typedef enum LorMmapMode {
    LOR_MMAP_READ = 0,
    LOR_MMAP_COPY = 1,
    LOR_MMAP_SHARED = 2
} LorMmapMode;

typedef struct LorMmap {
    void *data;
    size_t size;
} LorMmap;

LorMmap lor_mmap_file(const char *path, LorMmapMode mode);
void lor_mmap_unmap(LorMmap *map);

typedef struct LorLeakStats {
    size_t heap_count;
    size_t heap_bytes;
    size_t arena_count;
    size_t virtual_count;
    size_t virtual_bytes;
    size_t mmap_count;
    size_t mmap_bytes;
} LorLeakStats;

void lor_leakcheck_enable(int enabled);
int lor_leakcheck_enabled(void);
LorLeakStats lor_leakcheck_stats(void);
size_t lor_leakcheck_count(void);
size_t lor_leakcheck_report(FILE *out);

void *lor_malloc(size_t size);
void *lor_calloc(size_t count, size_t elem_size);
void *lor_realloc(void *ptr, size_t size);
void lor_free(void *ptr);
char *lor_strdup(const char *text);

void *lor_malloc_debug(size_t size, const char *file, int line);
void *lor_calloc_debug(size_t count, size_t elem_size, const char *file, int line);
void *lor_realloc_debug(void *ptr, size_t size, const char *file, int line);
void lor_free_debug(void *ptr, const char *file, int line);
char *lor_strdup_debug(const char *text, const char *file, int line);

void lor_cleanup_free(void *ptr);
void lor_cleanup_arena(void *arena);
void lor_cleanup_arena_temp(void *temp);
void lor_cleanup_mmap(void *map);
void lor_cleanup_virtual(void *memory);

#if defined(__GNUC__) || defined(__clang__)
#define LOR_CLEANUP_SUPPORTED 1
#define LOR_CLEANUP(fn) __attribute__((cleanup(fn)))
#else
#define LOR_CLEANUP_SUPPORTED 0
#define LOR_CLEANUP(fn)
#endif

#define LOR_AUTO_FREE LOR_CLEANUP(lor_cleanup_free)
#define LOR_AUTO_ARENA LOR_CLEANUP(lor_cleanup_arena)
#define LOR_AUTO_ARENA_TEMP LOR_CLEANUP(lor_cleanup_arena_temp)
#define LOR_AUTO_MMAP LOR_CLEANUP(lor_cleanup_mmap)
#define LOR_AUTO_VIRTUAL LOR_CLEANUP(lor_cleanup_virtual)

#ifdef __cplusplus
}
#endif

/* Optional macro mode. Define LOR_LEAKCHECK_STDLIB before including this
   header in a translation unit to route standard heap calls through liblor's
   leak tracker. Use consistently inside that translation unit. */
#if defined(LOR_LEAKCHECK_STDLIB) && !defined(LOR_MEMORY_NO_STDLIB_MACROS) && \
    !defined(LOR_SINGLE_HEADER_BUILD)
#define malloc(size) lor_malloc_debug((size), __FILE__, __LINE__)
#define calloc(count, elem_size) \
    lor_calloc_debug((count), (elem_size), __FILE__, __LINE__)
#define realloc(ptr, size) lor_realloc_debug((ptr), (size), __FILE__, __LINE__)
#define free(ptr) lor_free_debug((ptr), __FILE__, __LINE__)
#define strdup(text) lor_strdup_debug((text), __FILE__, __LINE__)
#endif

#endif
