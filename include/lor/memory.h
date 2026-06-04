// SPDX-License-Identifier: MIT

#ifndef LOR_MEMORY_H
#define LOR_MEMORY_H

#include <stddef.h>
#include <stdbool.h>
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

typedef enum {
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

typedef struct LorArenaAllocOptions {
    bool zero;
} LorArenaAllocOptions;

typedef struct LorArena {
    LorArenaBlock *blocks;
    LorArenaBackend backend;
    size_t block_size;
    size_t reserve_size;  // capacity
    size_t commit_size;   // allocation size
} LorArena;

typedef struct LorArenaMark {
    LorArenaBlock *block;
    size_t used;
} LorArenaMark;

typedef struct LorScratch {
    LorArena *arena;
    LorArenaMark mark;
} LorScratch;

#define LOR_ARENA_INIT {NULL, LOR_ARENA_BACKEND_HEAP, 0u, 0u, 0u}
#define LOR_ARENA_MARK_INIT {NULL, 0u}
#define LOR_SCRATCH_INIT {NULL, LOR_ARENA_MARK_INIT}

int lor_memory_arena_init_(LorArena *arena, const LorArenaConfig *config);

/** Releases all storage owned by `arena` and resets it to `LOR_ARENA_INIT`.
 *
 * Passing `NULL` is a no-op. All pointers previously allocated from the arena
 * become invalid.
 */
void lor_arena_deinit(LorArena *arena);

/** Rewinds all allocations in `arena` while keeping its allocated blocks.
 *
 * Passing `NULL` is a no-op. This retains the arena's high-water storage for
 * reuse; call `lor_arena_deinit` to release storage back to the system.
 */
void lor_arena_reset(LorArena *arena);

/** Returns a checkpoint for temporary allocations inside `arena`.
 *
 * The mark is a plain value and should be passed back to `lor_arena_rewind`
 * with the same arena. Passing `NULL` returns `LOR_ARENA_MARK_INIT`.
 */
LorArenaMark lor_arena_mark(const LorArena *arena);

/** Rewinds `arena` to `mark`.
 *
 * Blocks allocated after a non-empty mark are released. A zero mark rewinds all
 * allocations while retaining allocated blocks. Passing `NULL` is a no-op.
 * Marks must come from the same arena.
 */
void lor_arena_rewind(LorArena *arena, LorArenaMark mark);

void *lor_memory_arena_alloc_(LorArena *arena, size_t size,
                              LorArenaAllocOptions options);
void *lor_memory_arena_alloc_array_(LorArena *arena, size_t count,
                                    size_t elem_size,
                                    LorArenaAllocOptions options);

/** Copies the NUL-terminated string `text` into `arena`.
 *
 * Returns the arena-owned copy, or `NULL` when `text` is `NULL` or allocation
 * fails.
 */
char *lor_arena_strdup(LorArena *arena, const char *text);

/** Returns the total number of bytes currently used by allocations in `arena`. */
size_t lor_arena_used(const LorArena *arena);

/** Returns the total usable capacity currently held by `arena`.
 *
 * For virtual arenas this is reserved usable capacity, which can be larger than
 * the currently committed backing memory reported by `lor_arena_committed`.
 */
size_t lor_arena_capacity(const LorArena *arena);

/** Returns the total committed backing memory currently held by `arena`. */
size_t lor_arena_committed(const LorArena *arena);

/** Begins a scratch scope and returns a handle for a thread-local scratch arena.
 *
 * `conflicts` may name arenas whose live allocations must not be overwritten by
 * the new scratch scope. Returns `LOR_SCRATCH_INIT` when no scratch arena is
 * available or initialization fails.
 */
LorScratch lor_scratch_begin(LorArena **conflicts, size_t conflict_count);

/** Ends a scratch scope and rewinds its borrowed arena.
 *
 * Passing an inactive handle is a no-op.
 */
void lor_scratch_end(LorScratch scratch);

/** Releases all initialized scratch arenas for the current thread. */
void lor_scratch_cleanup(void);

/** Returns the host operating system page size, or a conservative fallback. */
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

/** Maps the file at `path` using `mode`.
 *
 * Returns `{0}` when `path` is `NULL`, the file cannot be opened, the file is
 * empty or too large, or the mapping fails. Successful mappings must be released
 * with `lor_mmap_unmap`.
 */
LorMmap lor_mmap_file(const char *path, LorMmapMode mode);

/** Unmaps `map` and resets it to `{0}`.
 *
 * Passing `NULL` or an already-unmapped value is a no-op.
 */
void lor_mmap_unmap(LorMmap *map);

typedef struct LorLeakStats {
    size_t heap_count;
    size_t heap_bytes;
    size_t arena_count;
    size_t mmap_count;
    size_t mmap_bytes;
} LorLeakStats;

/** Returns current leakcheck counters.
 *
 * In normal builds, all counters are zero. Leakcheck tracking is process-global
 * and not synchronized; use it from one thread or protect calls externally.
 */
LorLeakStats lor_leakcheck_stats(void);

/** Returns the number of currently tracked leakcheck records.
 *
 * In normal builds, this returns zero.
 */
size_t lor_leakcheck_count(void);

/** Writes current leakcheck records to `out` and returns the number written.
 *
 * Passing `out == NULL` writes to `stderr`. In normal builds, this writes
 * nothing and returns zero.
 */
size_t lor_leakcheck_report(FILE *out);

#if defined(LOR_LEAKCHECK)
/** Leakcheck wrapper for `malloc` with explicit source location metadata. */
void *lor_malloc_debug(size_t size, const char *file, int line);

/** Leakcheck wrapper for `calloc` with explicit source location metadata. */
void *lor_calloc_debug(size_t count, size_t elem_size, const char *file, int line);

/** Leakcheck wrapper for `realloc` with explicit source location metadata. */
void *lor_realloc_debug(void *ptr, size_t size, const char *file, int line);

/** Leakcheck wrapper for `free` with explicit source location metadata. */
void lor_free_debug(void *ptr, const char *file, int line);

/** Leakcheck wrapper for `strdup` with explicit source location metadata. */
char *lor_strdup_debug(const char *text, const char *file, int line);

int lor_memory_arena_init_debug_(LorArena *arena, const LorArenaConfig *config,
                                 const char *file, int line);

void *lor_memory_arena_alloc_debug_(LorArena *arena, size_t size,
                                    LorArenaAllocOptions options,
                                    const char *file, int line);
void *lor_memory_arena_alloc_array_debug_(LorArena *arena, size_t count,
                                          size_t elem_size,
                                          LorArenaAllocOptions options,
                                          const char *file, int line);

/** Leakcheck-tracked form of `lor_arena_strdup` with explicit source location. */
char *lor_arena_strdup_debug(LorArena *arena, const char *text,
                             const char *file, int line);

/** Leakcheck-tracked form of `lor_mmap_file` with explicit source location. */
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
    *value = (LorScratch)LOR_SCRATCH_INIT;
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

/* LOR_SINGLE_HEADER_LATE_MACROS_BEGIN */
/* Optional macro mode.
   These wrappers live after declarations in normal headers, and are emitted
   after implementation in the generated single-header. */
#if !defined(LOR_MEMORY_INTERNAL) && !defined(LOR_SINGLE_HEADER_BUILD)
#define LOR_MEMORY_SELECT_INIT_(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define LOR_MEMORY_ARENA_INIT_DEFAULT_(arena) \
    lor_memory_arena_init_((arena), NULL)
#define LOR_MEMORY_ARENA_INIT_OPTIONS_(arena, ...) \
    lor_memory_arena_init_((arena), &(LorArenaConfig){__VA_ARGS__})
#define LOR_MEMORY_ARENA_INIT_DEBUG_DEFAULT_(arena) \
    lor_memory_arena_init_debug_((arena), NULL, __FILE__, __LINE__)
#define LOR_MEMORY_ARENA_INIT_DEBUG_OPTIONS_(arena, ...) \
    lor_memory_arena_init_debug_((arena), &(LorArenaConfig){__VA_ARGS__}, \
                                 __FILE__, __LINE__)

#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS)
#define LOR_MEMORY_ARENA_INIT_DEFAULT LOR_MEMORY_ARENA_INIT_DEBUG_DEFAULT_
#define LOR_MEMORY_ARENA_INIT_OPTIONS LOR_MEMORY_ARENA_INIT_DEBUG_OPTIONS_
#else
#define LOR_MEMORY_ARENA_INIT_DEFAULT LOR_MEMORY_ARENA_INIT_DEFAULT_
#define LOR_MEMORY_ARENA_INIT_OPTIONS LOR_MEMORY_ARENA_INIT_OPTIONS_
#endif

#define LOR_MEMORY_ARENA_INIT_(...)                                      \
    LOR_MEMORY_SELECT_INIT_(__VA_ARGS__, LOR_MEMORY_ARENA_INIT_OPTIONS,   \
                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \
                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \
                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \
                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \
                            LOR_MEMORY_ARENA_INIT_DEFAULT, unused)        \
    (__VA_ARGS__)

/** Initializes an arena from optional designated configuration arguments.
 *
 * Supported forms:
 * `lor_arena_init(&arena)`
 * `lor_arena_init(&arena, .block_size = 128)`
 * `lor_arena_init(&arena, .backend = LOR_ARENA_BACKEND_VIRTUAL)`
 */
#undef lor_arena_init
#define lor_arena_init(...) LOR_MEMORY_ARENA_INIT_(__VA_ARGS__)

#define LOR_MEMORY_SELECT_ALLOC_(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define LOR_MEMORY_ARENA_ALLOC_DEFAULT_(arena, size) \
    lor_memory_arena_alloc_((arena), (size), (LorArenaAllocOptions){0})
#define LOR_MEMORY_ARENA_ALLOC_OPTIONS_(arena, size, ...) \
    lor_memory_arena_alloc_((arena), (size), (LorArenaAllocOptions){__VA_ARGS__})
#define LOR_MEMORY_ARENA_ALLOC_DEBUG_DEFAULT_(arena, size) \
    lor_memory_arena_alloc_debug_((arena), (size), (LorArenaAllocOptions){0}, \
                                  __FILE__, __LINE__)
#define LOR_MEMORY_ARENA_ALLOC_DEBUG_OPTIONS_(arena, size, ...) \
    lor_memory_arena_alloc_debug_((arena), (size),                       \
                                  (LorArenaAllocOptions){__VA_ARGS__},   \
                                  __FILE__, __LINE__)

#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS)
#define LOR_MEMORY_ARENA_ALLOC_DEFAULT LOR_MEMORY_ARENA_ALLOC_DEBUG_DEFAULT_
#define LOR_MEMORY_ARENA_ALLOC_OPTIONS LOR_MEMORY_ARENA_ALLOC_DEBUG_OPTIONS_
#else
#define LOR_MEMORY_ARENA_ALLOC_DEFAULT LOR_MEMORY_ARENA_ALLOC_DEFAULT_
#define LOR_MEMORY_ARENA_ALLOC_OPTIONS LOR_MEMORY_ARENA_ALLOC_OPTIONS_
#endif

#define LOR_MEMORY_ARENA_ALLOC_(...)                                      \
    LOR_MEMORY_SELECT_ALLOC_(__VA_ARGS__, LOR_MEMORY_ARENA_ALLOC_OPTIONS,  \
                             LOR_MEMORY_ARENA_ALLOC_OPTIONS,               \
                             LOR_MEMORY_ARENA_ALLOC_OPTIONS,               \
                             LOR_MEMORY_ARENA_ALLOC_OPTIONS,               \
                             LOR_MEMORY_ARENA_ALLOC_DEFAULT, unused)       \
    (__VA_ARGS__)

/** Allocates from an arena with optional designated allocation arguments.
 *
 * Supported forms:
 * `lor_arena_alloc(&arena, size)`
 * `lor_arena_alloc(&arena, size, .zero = true)`
 */
#undef lor_arena_alloc
#define lor_arena_alloc(...) LOR_MEMORY_ARENA_ALLOC_(__VA_ARGS__)

#define LOR_MEMORY_SELECT_ALLOC_ARRAY_(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT_(arena, count, elem_size)       \
    lor_memory_arena_alloc_array_((arena), (count), (elem_size),             \
                                  (LorArenaAllocOptions){0})
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS_(arena, count, elem_size, ...) \
    lor_memory_arena_alloc_array_((arena), (count), (elem_size),            \
                                  (LorArenaAllocOptions){__VA_ARGS__})
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_DEBUG_DEFAULT_(arena, count, elem_size) \
    lor_memory_arena_alloc_array_debug_((arena), (count), (elem_size),       \
                                        (LorArenaAllocOptions){0}, __FILE__, \
                                        __LINE__)
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_DEBUG_OPTIONS_(arena, count, elem_size, ...) \
    lor_memory_arena_alloc_array_debug_((arena), (count), (elem_size),            \
                                        (LorArenaAllocOptions){__VA_ARGS__},      \
                                        __FILE__, __LINE__)

#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS)
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT \
    LOR_MEMORY_ARENA_ALLOC_ARRAY_DEBUG_DEFAULT_
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS \
    LOR_MEMORY_ARENA_ALLOC_ARRAY_DEBUG_OPTIONS_
#else
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT_
#define LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS_
#endif

#define LOR_MEMORY_ARENA_ALLOC_ARRAY_(...)                                  \
    LOR_MEMORY_SELECT_ALLOC_ARRAY_(__VA_ARGS__,                             \
                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS,    \
                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS,    \
                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS,    \
                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT,    \
                                   unused)                                  \
    (__VA_ARGS__)

/** Allocates an array from an arena with optional designated arguments.
 *
 * Supported forms:
 * `lor_arena_alloc_array(&arena, count, sizeof(*items))`
 * `lor_arena_alloc_array(&arena, count, sizeof(*items), .zero = true)`
 */
#undef lor_arena_alloc_array
#define lor_arena_alloc_array(...) LOR_MEMORY_ARENA_ALLOC_ARRAY_(__VA_ARGS__)
#endif

/* Leakcheck build mode. Define LOR_LEAKCHECK for the whole build to route
   liblor memory calls and standard heap calls through location-aware tracking. */
#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS) && \
    !defined(LOR_SINGLE_HEADER_BUILD)
#undef lor_arena_strdup
#define lor_arena_strdup(arena, text) \
    lor_arena_strdup_debug((arena), (text), __FILE__, __LINE__)
#undef lor_mmap_file
#define lor_mmap_file(path, mode) \
    lor_mmap_file_debug((path), (mode), __FILE__, __LINE__)
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
/* LOR_SINGLE_HEADER_LATE_MACROS_END */

#endif
