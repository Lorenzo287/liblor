// SPDX-License-Identifier: MIT

#ifndef LOR_MEMORY_H
#define LOR_MEMORY_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    // Selects which backing storage the arena uses.
    LorArenaBackend backend;
    // Preferred usable capacity of each heap block. Ignored when virtual.
    size_t block_size;
    // Preferred address-space reservation per virtual block. Ignored when heap.
    size_t reserve_size;
    // Virtual-memory commit granularity. Ignored when heap.
    size_t commit_size;
} LorArenaConfig;

typedef struct LorArena {
    // Blocks owned by the arena, newest first. Shared by both backends.
    LorArenaBlock *blocks;
    // Determines how blocks are acquired, grown, and released.
    LorArenaBackend backend;
    // Preferred heap block capacity. Ignored by the virtual backend.
    size_t block_size;
    // Preferred virtual block reservation. Ignored by the heap backend.
    size_t reserve_size;
    // Virtual-memory commit granularity. Ignored by the heap backend.
    size_t commit_size;
} LorArena;

typedef struct LorArenaMark {
    // Block that was active when the mark was taken.
    LorArenaBlock *block;
    // Used offset in `block` when the mark was taken.
    size_t used;
} LorArenaMark;

typedef struct LorScratch {
    LorArena *arena;
    LorArenaMark mark;
} LorScratch;

#define LOR_ARENA_INIT {NULL, LOR_ARENA_BACKEND_HEAP, 0u, 0u, 0u}
#define LOR_ARENA_MARK_INIT {NULL, 0u}
#define LOR_SCRATCH_INIT {NULL, LOR_ARENA_MARK_INIT}

/* Initializes `arena` with the default heap-backed configuration.

   Passing `NULL` fails and returns zero. A zero-initialized arena can also be
   used lazily by calling `lor_arena_alloc` without an explicit init call. */
int lor_arena_init(LorArena *arena);

/* Initializes `arena` with explicit configuration.

   Fields left as zero use the same defaults as `lor_arena_init`. Use
   `LOR_ARENA_BACKEND_VIRTUAL` to reserve virtual memory and commit it on demand. */
int lor_arena_init_config(LorArena *arena, LorArenaConfig config);

/* Releases all storage owned by `arena` and resets it to `LOR_ARENA_INIT`.

   Passing `NULL` is a no-op. All pointers previously allocated from the arena
   become invalid. */
void lor_arena_deinit(LorArena *arena);

/* Rewinds all allocations in `arena` while keeping its allocated blocks.

   Passing `NULL` is a no-op. This retains the arena's high-water storage for
   reuse; call `lor_arena_deinit` to release storage back to the system. */
void lor_arena_reset(LorArena *arena);

/* Returns a checkpoint for temporary allocations inside `arena`.

   The mark is a plain value and should be passed back to `lor_arena_rewind`
   with the same arena. Passing `NULL` returns `LOR_ARENA_MARK_INIT`. */
LorArenaMark lor_arena_mark(const LorArena *arena);

/* Rewinds `arena` to `mark`.

   Blocks allocated after a non-empty mark are released. A zero mark rewinds all
   allocations while retaining allocated blocks. Passing `NULL` is a no-op.
   Marks must come from the same arena. */
void lor_arena_rewind(LorArena *arena, LorArenaMark mark);

/* Allocates `size` bytes from `arena`.

   Returns `NULL` when `arena` is `NULL`, `size` is zero, or backing allocation
   fails. The returned pointer is owned by the arena and must not be freed
   individually. */
void *lor_arena_alloc(LorArena *arena, size_t size);

// Allocates zero-filled `size` bytes from `arena`.
void *lor_arena_alloc_zero(LorArena *arena, size_t size);

/* Allocates `count * elem_size` bytes from `arena`.

   Returns `NULL` for zero counts, zero element sizes, or size overflow. */
void *lor_arena_alloc_array(LorArena *arena, size_t count, size_t elem_size);

// Allocates a zero-filled array from `arena`.
void *lor_arena_alloc_array_zero(LorArena *arena, size_t count, size_t elem_size);

/* Copies the NUL-terminated string `text` into `arena`.

   Returns the arena-owned copy, or `NULL` when `text` is `NULL` or allocation
   fails. */
char *lor_arena_strdup(LorArena *arena, const char *text);

/* Returns the usable capacity currently consumed in `arena`.

   The result includes alignment padding between allocations, so it can exceed
   the sum of requested allocation sizes. Resetting or rewinding lowers this
   value without necessarily lowering capacity or committed memory. Passing
   `NULL` returns zero. */
size_t lor_arena_used(const LorArena *arena);

/* Returns the total usable capacity currently held by `arena`.

   Block metadata and alignment slack are excluded. For heap arenas every byte
   of capacity is backed by an allocation, so this equals
   `lor_arena_committed`. For virtual arenas this is reserved usable
   address-space capacity and can be larger than committed memory. Passing
   `NULL` returns zero. */
size_t lor_arena_capacity(const LorArena *arena);

/* Returns the usable capacity currently backed by committed memory.

   Block metadata and alignment slack are excluded. This equals
   `lor_arena_capacity` for heap arenas. For virtual arenas it grows on demand
   in commit-size increments and is not reduced by reset or rewind within a
   retained block. Passing `NULL` returns zero. */
size_t lor_arena_committed(const LorArena *arena);

/* Begins a scratch scope and returns a handle for a thread-local scratch arena.

   `conflicts` may name arenas whose live allocations must not be overwritten by
   the new scratch scope. Returns `LOR_SCRATCH_INIT` when no scratch arena is
   available or initialization fails. */
LorScratch lor_scratch_begin(LorArena **conflicts, size_t conflict_count);

/* Ends a scratch scope and rewinds its borrowed arena.

   Passing an inactive handle is a no-op. */
void lor_scratch_end(LorScratch scratch);

// Releases all initialized scratch arenas for the current thread.
void lor_scratch_cleanup(void);

// Returns the host operating system page size, or a conservative fallback.
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

/* Maps the file at `path` using `mode`.

   Returns `{0}` when `path` is `NULL`, the file cannot be opened, the file is
   empty or too large, or the mapping fails. Successful mappings must be released
   with `lor_mmap_unmap`. */
LorMmap lor_mmap_file(const char *path, LorMmapMode mode);

/* Unmaps `map` and resets it to `{0}`.

   Passing `NULL` or an already-unmapped value is a no-op. */
void lor_mmap_unmap(LorMmap *map);

typedef struct LorLeakStats {
    size_t heap_count;
    size_t heap_bytes;
    size_t arena_count;
    size_t mmap_count;
    size_t mmap_bytes;
} LorLeakStats;

/* Returns current leakcheck counters.

   In normal builds, all counters are zero. Leakcheck tracking is process-global
   and not synchronized; use it from one thread or protect calls externally. */
LorLeakStats lor_leakcheck_stats(void);

/* Returns the number of currently tracked leakcheck records.

   In normal builds, this returns zero. */
size_t lor_leakcheck_count(void);

/* Writes current leakcheck records to `out` and returns the number written.

   Passing `out == NULL` writes to `stderr`. In normal builds, this writes
   nothing and returns zero. */
size_t lor_leakcheck_report(FILE *out);

#if defined(LOR_LEAKCHECK)
// Leakcheck wrapper for `malloc` with explicit source location metadata.
void *lor_malloc_debug(size_t size, const char *file, int line);

// Leakcheck wrapper for `calloc` with explicit source location metadata.
void *lor_calloc_debug(size_t count, size_t elem_size, const char *file, int line);

// Leakcheck wrapper for `realloc` with explicit source location metadata.
void *lor_realloc_debug(void *ptr, size_t size, const char *file, int line);

// Leakcheck wrapper for `free` with explicit source location metadata.
void lor_free_debug(void *ptr, const char *file, int line);

// Leakcheck wrapper for `strdup` with explicit source location metadata.
char *lor_strdup_debug(const char *text, const char *file, int line);

// Leakcheck-tracked form of `lor_arena_init` with explicit source location.
int lor_arena_init_debug(LorArena *arena, const char *file, int line);

// Leakcheck-tracked form of `lor_arena_init_config`.
int lor_arena_init_config_debug(LorArena *arena, LorArenaConfig config,
                                const char *file, int line);

/* Leakcheck-tracked form of `lor_arena_alloc`.

   Arena allocations are still bulk-owned by the arena; this location is used
   only when a zero-initialized arena is lazily initialized by the allocation. */
void *lor_arena_alloc_debug(LorArena *arena, size_t size, const char *file,
                            int line);

// Leakcheck-tracked form of `lor_arena_alloc_zero`.
void *lor_arena_alloc_zero_debug(LorArena *arena, size_t size, const char *file,
                                 int line);

// Leakcheck-tracked form of `lor_arena_alloc_array`.
void *lor_arena_alloc_array_debug(LorArena *arena, size_t count, size_t elem_size,
                                  const char *file, int line);

// Leakcheck-tracked form of `lor_arena_alloc_array_zero`.
void *lor_arena_alloc_array_zero_debug(LorArena *arena, size_t count,
                                       size_t elem_size, const char *file, int line);

// Leakcheck-tracked form of `lor_arena_strdup` with explicit source location.
char *lor_arena_strdup_debug(LorArena *arena, const char *text, const char *file,
                             int line);

// Leakcheck-tracked form of `lor_mmap_file` with explicit source location.
LorMmap lor_mmap_file_debug(const char *path, LorMmapMode mode, const char *file,
                            int line);
#endif

/* Scope cleanup has to be a macro because C attributes are declaration syntax.
   Unsupported compilers leave LOR_AUTO_* empty, so code remains portable. */
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
    if (value == NULL || *value == NULL) return;
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
    if (value == NULL || value->arena == NULL) return;
    lor_scratch_end(*value);
    *value = (LorScratch)LOR_SCRATCH_INIT;
}

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_mmap_(void *map) {
    lor_mmap_unmap((LorMmap *)map);
}

static inline void LOR_MAYBE_UNUSED lor_memory_cleanup_file_(void *file) {
    FILE **value = (FILE **)file;
    if (value == NULL || *value == NULL) return;
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

// LOR_SINGLE_HEADER_LATE_MACROS_BEGIN
/* Leakcheck location capture.

   These macros are deliberately kept at the end of the header. In normal
   multi-file builds they affect user code after all declarations are visible.
   In the generated single-header they are emitted after the implementation, so
   they do not rewrite liblor's own function definitions.

   src/memory.c defines LOR_MEMORY_NO_LOCATION_MACROS and
   LOR_MEMORY_NO_STDLIB_MACROS before including this header because the
   implementation needs to define and call the real functions. */
#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS) && \
    !defined(LOR_SINGLE_HEADER_BUILD)
#undef lor_arena_init
#define lor_arena_init(arena) lor_arena_init_debug((arena), __FILE__, __LINE__)
#undef lor_arena_init_config
#define lor_arena_init_config(arena, ...) \
    lor_arena_init_config_debug((arena), __VA_ARGS__, __FILE__, __LINE__)
#undef lor_arena_alloc
#define lor_arena_alloc(arena, size) \
    lor_arena_alloc_debug((arena), (size), __FILE__, __LINE__)
#undef lor_arena_alloc_zero
#define lor_arena_alloc_zero(arena, size) \
    lor_arena_alloc_zero_debug((arena), (size), __FILE__, __LINE__)
#undef lor_arena_alloc_array
#define lor_arena_alloc_array(arena, count, elem_size) \
    lor_arena_alloc_array_debug((arena), (count), (elem_size), __FILE__, __LINE__)
#undef lor_arena_alloc_array_zero
#define lor_arena_alloc_array_zero(arena, count, elem_size)                   \
    lor_arena_alloc_array_zero_debug((arena), (count), (elem_size), __FILE__, \
                                     __LINE__)
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
// LOR_SINGLE_HEADER_LATE_MACROS_END

#endif
