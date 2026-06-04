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
typedef struct LorArenaScope LorArenaScope;

typedef struct LorArenaConfig {
    LorArenaBackend backend;
    size_t block_size;
    size_t reserve_size;
    size_t commit_size;
} LorArenaConfig;

typedef struct LorArena {
    LorArenaBlock *blocks;
    LorArenaScope *scopes;
    LorArenaBackend backend;
    size_t block_size;
    size_t reserve_size;  // capacity
    size_t commit_size;   // allocation size
} LorArena;

typedef struct LorScratch {
    LorArena *arena;
    LorArenaBlock *block;
    size_t used;
} LorScratch;

#define LOR_ARENA_INIT {NULL, NULL, LOR_ARENA_BACKEND_HEAP, 0u, 0u, 0u}

void lor_arena_init(LorArena *arena, size_t block_size);
int lor_arena_init_ex(LorArena *arena, const LorArenaConfig *config);
void lor_arena_deinit(LorArena *arena);
void lor_arena_reset(LorArena *arena);

int lor_arena_mark(LorArena *arena);
void lor_arena_rewind(LorArena *arena);

void *lor_arena_alloc(LorArena *arena, size_t size);
void *lor_arena_alloc_zero(LorArena *arena, size_t size);
void *lor_arena_alloc_array(LorArena *arena, size_t count, size_t elem_size);
void *lor_arena_alloc_array_zero(LorArena *arena, size_t count, size_t elem_size);
char *lor_arena_strdup(LorArena *arena, const char *text);

size_t lor_arena_used(const LorArena *arena);
size_t lor_arena_capacity(const LorArena *arena);
size_t lor_arena_committed(const LorArena *arena);

LorScratch lor_scratch_begin(LorArena **conflicts, size_t conflict_count);
void lor_scratch_end(LorScratch scratch);
void lor_scratch_cleanup(void);

size_t lor_page_size(void);

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
    size_t mmap_count;
    size_t mmap_bytes;
} LorLeakStats;

LorLeakStats lor_leakcheck_stats(void);
size_t lor_leakcheck_count(void);
size_t lor_leakcheck_report(FILE *out);

#if defined(LOR_LEAKCHECK)
void *lor_malloc_debug(size_t size, const char *file, int line);
void *lor_calloc_debug(size_t count, size_t elem_size, const char *file, int line);
void *lor_realloc_debug(void *ptr, size_t size, const char *file, int line);
void lor_free_debug(void *ptr, const char *file, int line);
char *lor_strdup_debug(const char *text, const char *file, int line);
void lor_arena_init_debug(LorArena *arena, size_t block_size, const char *file,
                          int line);
int lor_arena_init_ex_debug(LorArena *arena, const LorArenaConfig *config,
                            const char *file, int line);
LorMmap lor_mmap_file_debug(const char *path, LorMmapMode mode, const char *file,
                            int line);
#endif

#if defined(__GNUC__) || defined(__clang__)
#define LOR_CLEANUP_SUPPORTED 1
#define LOR_CLEANUP(fn) __attribute__((cleanup(fn)))
#define LOR_MAYBE_UNUSED __attribute__((unused))
#else
#define LOR_CLEANUP_SUPPORTED 0
#define LOR_CLEANUP(fn)
#define LOR_MAYBE_UNUSED
#endif

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_free_(void *ptr) {
    void **value = (void **)ptr;
    if (value == NULL || *value == NULL) { return; }
#if defined(LOR_LEAKCHECK)
    lor_free_debug(*value, NULL, 0);
#else
    free(*value);
#endif
    *value = NULL;
}

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_arena_(void *arena) {
    lor_arena_deinit((LorArena *)arena);
}

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_scratch_(void *scratch) {
    LorScratch *value = (LorScratch *)scratch;
    if (value == NULL || value->arena == NULL) { return; }
    lor_scratch_end(*value);
    value->arena = NULL;
}

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_mmap_(void *map) {
    lor_mmap_unmap((LorMmap *)map);
}

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_file_(void *file) {
    FILE **value = (FILE **)file;
    if (value == NULL || *value == NULL) { return; }
    (void)fclose(*value);
    *value = NULL;
}

#define LOR_AUTO_FREE LOR_CLEANUP(lor_memory_cleanup_free_)
#define LOR_AUTO_ARENA LOR_CLEANUP(lor_memory_cleanup_arena_)
#define LOR_AUTO_SCRATCH LOR_CLEANUP(lor_memory_cleanup_scratch_)
#define LOR_AUTO_MMAP LOR_CLEANUP(lor_memory_cleanup_mmap_)
#define LOR_AUTO_FILE LOR_CLEANUP(lor_memory_cleanup_file_)

#ifdef __cplusplus
}
#endif

/* Leakcheck build mode. Define LOR_LEAKCHECK for the whole build to route
   liblor memory calls and standard heap calls through location-aware tracking. */
#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS) && \
    !defined(LOR_SINGLE_HEADER_BUILD)
#define lor_arena_init(arena, block_size) \
    lor_arena_init_debug((arena), (block_size), __FILE__, __LINE__)
#define lor_arena_init_ex(arena, config) \
    lor_arena_init_ex_debug((arena), (config), __FILE__, __LINE__)
#define lor_mmap_file(path, mode) lor_mmap_file_debug((path), (mode), __FILE__, __LINE__)
#endif

#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_STDLIB_MACROS) && \
    !defined(LOR_SINGLE_HEADER_BUILD)
#define malloc(size) lor_malloc_debug((size), __FILE__, __LINE__)
#define calloc(count, elem_size) \
    lor_calloc_debug((count), (elem_size), __FILE__, __LINE__)
#define realloc(ptr, size) lor_realloc_debug((ptr), (size), __FILE__, __LINE__)
#define free(ptr) lor_free_debug((ptr), __FILE__, __LINE__)
#define strdup(text) lor_strdup_debug((text), __FILE__, __LINE__)
#endif

#endif
