// SPDX-License-Identifier: MIT

/* The implementation needs the real function names. In leakcheck builds the
   public header can macro-wrap those names to attach call-site file/line data. */
#define LOR_MEMORY_NO_LOCATION_MACROS
#define LOR_MEMORY_NO_STDLIB_MACROS
#include "lor/memory.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define LOR_ARENA_MAX_ALIGNMENT alignof(max_align_t)
#else
#define LOR_ARENA_MAX_ALIGNMENT \
    (sizeof(void *) > sizeof(double) ? sizeof(void *) : sizeof(double))
#endif

#if defined(_MSC_VER)
#define LOR_THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define LOR_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
#define LOR_THREAD_LOCAL __thread
#else
#define LOR_THREAD_LOCAL
#endif

typedef enum LorLeakKind {
    LOR_LEAK_KIND_HEAP,
    LOR_LEAK_KIND_ARENA,
    LOR_LEAK_KIND_MMAP
} LorLeakKind;

#if defined(LOR_LEAKCHECK)
typedef struct LorLeakRecord LorLeakRecord;

struct LorLeakRecord {
    LorLeakRecord *next;
    LorLeakRecord *prev;
    LorLeakKind kind;
    void *ptr;
    size_t size;
    const char *file;
    int line;
};
#endif

struct LorArenaBlock {
    // Newer blocks precede older blocks in the arena's allocation history.
    LorArenaBlock *next;
    // Bytes available from the aligned data start.
    size_t capacity;
    /* Heap: usable bytes backed by the allocation, equal to capacity.
       Virtual: bytes committed from the raw block base, including metadata. */
    size_t committed;
    // Consumed usable bytes, including inter-allocation alignment padding.
    size_t used;
    /* Heap: raw allocation size. Virtual: raw reserved address-space size.
       Both include the block metadata and data-alignment slack. */
    size_t allocation_size;
    // Unaligned start marker; lor_arena__block_data returns the usable start.
    unsigned char data[];
};

#if defined(LOR_LEAKCHECK)
static LorLeakRecord *lor_memory__leaks = NULL;
#endif
static LOR_THREAD_LOCAL LorArena lor_memory__scratch_arenas[2];
static LOR_THREAD_LOCAL bool lor_memory__scratch_inited[2];

static void *lor_memory__raw_malloc(size_t size) {
    return malloc(size);
}

#if defined(LOR_LEAKCHECK)
static void *lor_memory__raw_calloc(size_t count, size_t elem_size) {
    return calloc(count, elem_size);
}

static void *lor_memory__raw_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}
#endif

static void lor_memory__raw_free(void *ptr) {
    free(ptr);
}

static int lor_memory__add_overflows_size(size_t a, size_t b) {
    return a > SIZE_MAX - b;
}

static int lor_memory__mul_overflows_size(size_t a, size_t b) {
    return b != 0 && a > SIZE_MAX / b;
}

static int lor_memory__is_power_of_two(size_t value) {
    return value != 0 && (value & (value - 1u)) == 0;
}

static size_t lor_memory__align_up(size_t value, size_t alignment) {
    size_t mask = alignment - 1u;
    return (value + mask) & ~mask;
}

static int lor_memory__align_forward(uintptr_t value, size_t alignment,
                                     uintptr_t *out) {
    uintptr_t mask = (uintptr_t)(alignment - 1u);
    if (value > UINTPTR_MAX - mask) return 0;

    *out = (value + mask) & ~mask;
    return 1;
}

#if defined(LOR_LEAKCHECK)
static void lor_leak__insert(LorLeakRecord *record) {
    record->prev = NULL;
    record->next = lor_memory__leaks;
    if (lor_memory__leaks != NULL) lor_memory__leaks->prev = record;
    lor_memory__leaks = record;
}

static void lor_leak__remove(LorLeakRecord *record) {
    if (record->prev != NULL) {
        record->prev->next = record->next;
    } else {
        lor_memory__leaks = record->next;
    }

    if (record->next != NULL) record->next->prev = record->prev;
    lor_memory__raw_free(record);
}

static LorLeakRecord *lor_leak__find(void *ptr, LorLeakKind kind) {
    for (LorLeakRecord *record = lor_memory__leaks; record != NULL;
         record = record->next)
        if (record->ptr == ptr && record->kind == kind) return record;

    return NULL;
}

static void lor_leak__track(LorLeakKind kind, void *ptr, size_t size,
                            const char *file, int line) {
    if (ptr == NULL) return;

    LorLeakRecord *record = (LorLeakRecord *)lor_memory__raw_malloc(sizeof(*record));
    if (record == NULL) return;
    record->kind = kind;
    record->ptr = ptr;
    record->size = size;
    record->file = file;
    record->line = line;
    lor_leak__insert(record);
}

static void lor_leak__untrack(LorLeakKind kind, void *ptr) {
    LorLeakRecord *record = lor_leak__find(ptr, kind);
    if (record != NULL) lor_leak__remove(record);
}

static const char *lor_leak__kind_name(LorLeakKind kind) {
    switch (kind) {
    case LOR_LEAK_KIND_HEAP:
        return "heap";
    case LOR_LEAK_KIND_ARENA:
        return "arena";
    case LOR_LEAK_KIND_MMAP:
        return "mmap";
    }

    return "unknown";
}

static size_t lor_leak__record_size(const LorLeakRecord *record) {
    if (record == NULL) return 0;
    if (record->kind == LOR_LEAK_KIND_ARENA)
        return lor_arena_committed((const LorArena *)record->ptr);
    return record->size;
}
#else
#define lor_leak__track(kind, ptr, size, file, line) ((void)0)
#define lor_leak__untrack(kind, ptr) ((void)0)
#endif

int lor_leakcheck_is_enabled(void) {
#if defined(LOR_LEAKCHECK)
    return 1;
#else
    return 0;
#endif
}

LorLeakStats lor_leakcheck_stats(void) {
    LorLeakStats stats = {0};
#if defined(LOR_LEAKCHECK)
    for (LorLeakRecord *record = lor_memory__leaks; record != NULL;
         record = record->next) {
        switch (record->kind) {
        case LOR_LEAK_KIND_HEAP:
            stats.heap_count += 1u;
            stats.heap_bytes += record->size;
            break;
        case LOR_LEAK_KIND_ARENA:
            stats.arena_count += 1u;
            break;
        case LOR_LEAK_KIND_MMAP:
            stats.mmap_count += 1u;
            stats.mmap_bytes += record->size;
            break;
        }
    }
#endif

    return stats;
}

size_t lor_leakcheck_count(void) {
    LorLeakStats stats = lor_leakcheck_stats();
    return stats.heap_count + stats.arena_count + stats.mmap_count;
}

size_t lor_leakcheck_report(FILE *out) {
    size_t count = 0;
#if defined(LOR_LEAKCHECK)
    if (out == NULL) out = stderr;

    for (LorLeakRecord *record = lor_memory__leaks; record != NULL;
         record = record->next) {
        const char *file = record->file != NULL ? record->file : "?";
        fprintf(out, "LEAK %s: %zu bytes at %p (%s:%d)\n",
                lor_leak__kind_name(record->kind), lor_leak__record_size(record),
                record->ptr, file, record->line);
        count += 1u;
    }
#else
    (void)out;
#endif

    return count;
}

#if defined(LOR_LEAKCHECK)
void *lor_malloc_debug(size_t size, const char *file, int line) {
    if (size == 0) return NULL;

    void *ptr = lor_memory__raw_malloc(size);
    lor_leak__track(LOR_LEAK_KIND_HEAP, ptr, size, file, line);
    return ptr;
}

void *lor_calloc_debug(size_t count, size_t elem_size, const char *file, int line) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size))
        return NULL;

    void *ptr = lor_memory__raw_calloc(count, elem_size);
    lor_leak__track(LOR_LEAK_KIND_HEAP, ptr, count * elem_size, file, line);
    return ptr;
}

void lor_free_debug(void *ptr, const char *file, int line) {
    (void)file;
    (void)line;
    if (ptr == NULL) return;

    lor_leak__untrack(LOR_LEAK_KIND_HEAP, ptr);
    lor_memory__raw_free(ptr);
}

void *lor_realloc_debug(void *ptr, size_t size, const char *file, int line) {
    if (ptr == NULL) return lor_malloc_debug(size, file, line);
    if (size == 0) {
        lor_free_debug(ptr, file, line);
        return NULL;
    }

    LorLeakRecord *record = lor_leak__find(ptr, LOR_LEAK_KIND_HEAP);
    void *new_ptr = lor_memory__raw_realloc(ptr, size);
    if (new_ptr == NULL) return NULL;

    if (record != NULL) {
        record->ptr = new_ptr;
        record->size = size;
        record->file = file;
        record->line = line;
    } else {
        lor_leak__track(LOR_LEAK_KIND_HEAP, new_ptr, size, file, line);
    }

    return new_ptr;
}

char *lor_strdup_debug(const char *text, const char *file, int line) {
    if (text == NULL) return NULL;

    size_t len = strlen(text);
    if (len == SIZE_MAX) return NULL;

    char *copy = (char *)lor_malloc_debug(len + 1u, file, line);
    if (copy == NULL) return NULL;

    memcpy(copy, text, len + 1u);
    return copy;
}
#endif

size_t lor_page_size(void) {
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (size_t)info.dwPageSize;
#else
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) return LOR_KIB(4);
    return (size_t)page_size;
#endif
}

static size_t lor_virtual__page_align(size_t size) {
    size_t page_size = lor_page_size();
    if (page_size == 0) return size;
    if (lor_memory__add_overflows_size(size, page_size - 1u)) return 0;
    return lor_memory__align_up(size, page_size);
}

static void *lor_virtual__reserve_raw(size_t size) {
#if defined(_WIN32)
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
#else
    void *ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr == MAP_FAILED ? NULL : ptr;
#endif
}

static int lor_virtual__commit_raw(void *ptr, size_t size) {
#if defined(_WIN32)
    return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
#else
    return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
#endif
}

static void lor_virtual__release_raw(void *ptr, size_t size) {
#if defined(_WIN32)
    (void)size;
    (void)VirtualFree(ptr, 0, MEM_RELEASE);
#else
    (void)munmap(ptr, size);
#endif
}

static unsigned char *lor_arena__block_data(LorArenaBlock *block) {
    uintptr_t aligned = 0;
    if (!lor_memory__align_forward((uintptr_t)block->data, LOR_ARENA_MAX_ALIGNMENT,
                                   &aligned))
        return NULL;

    return (unsigned char *)aligned;
}

static size_t lor_arena__block_header_slack(void) {
    return offsetof(LorArenaBlock, data) + LOR_ARENA_MAX_ALIGNMENT - 1u;
}

static LorArenaBlock *lor_arena__heap_block_new(size_t capacity) {
    size_t slack = lor_arena__block_header_slack();
    if (lor_memory__add_overflows_size(capacity, slack)) return NULL;

    size_t allocation_size = capacity + slack;
    LorArenaBlock *block = (LorArenaBlock *)lor_memory__raw_malloc(allocation_size);
    if (block == NULL) return NULL;

    block->next = NULL;
    block->capacity = capacity;
    block->committed = capacity;
    block->used = 0;
    block->allocation_size = allocation_size;
    return block;
}

static LorArenaBlock *lor_arena__virtual_block_new(size_t reserve_size,
                                                   size_t commit_size,
                                                   size_t min_capacity) {
    size_t slack = lor_arena__block_header_slack();
    if (lor_memory__add_overflows_size(min_capacity, slack)) return NULL;

    size_t min_reserve = min_capacity + slack;
    if (reserve_size < min_reserve) reserve_size = min_reserve;
    reserve_size = lor_virtual__page_align(reserve_size);
    if (reserve_size == 0) return NULL;

    if (commit_size < slack) commit_size = slack;
    if (commit_size < min_reserve && min_reserve < LOR_ARENA_DEFAULT_COMMIT_SIZE)
        commit_size = min_reserve;
    commit_size = lor_virtual__page_align(commit_size);
    if (commit_size > reserve_size) commit_size = reserve_size;

    LorArenaBlock *block = (LorArenaBlock *)lor_virtual__reserve_raw(reserve_size);
    if (block == NULL) return NULL;
    if (commit_size != 0 && !lor_virtual__commit_raw(block, commit_size)) {
        lor_virtual__release_raw(block, reserve_size);
        return NULL;
    }

    block->next = NULL;
    block->capacity = 0;
    block->committed = commit_size;
    block->used = 0;
    block->allocation_size = reserve_size;

    unsigned char *base = (unsigned char *)block;
    unsigned char *data = lor_arena__block_data(block);
    if (data == NULL) {
        lor_virtual__release_raw(block, reserve_size);
        return NULL;
    }

    size_t data_offset = (size_t)(data - base);
    block->capacity = reserve_size - data_offset;
    return block;
}

static void lor_arena__block_free(LorArena *arena, LorArenaBlock *block) {
    if (block == NULL) return;

    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL)
        lor_virtual__release_raw(block, block->allocation_size);
    else
        lor_memory__raw_free(block);
}

static int lor_arena__block_ensure_committed(LorArenaBlock *block,
                                             size_t needed_used,
                                             size_t commit_size) {
    unsigned char *base = lor_arena__block_data(block);
    unsigned char *block_base = (unsigned char *)block;
    if (base == NULL || needed_used > block->capacity) return 0;

    size_t data_offset = (size_t)(base - block_base);
    if (lor_memory__add_overflows_size(data_offset, needed_used)) return 0;

    size_t needed_total = data_offset + needed_used;
    if (needed_total <= block->committed) return 1;

    size_t new_committed = needed_total;
    if (commit_size != 0) {
        if (lor_memory__add_overflows_size(new_committed, commit_size - 1u))
            return 0;
        new_committed = lor_memory__align_up(new_committed, commit_size);
    }

    if (new_committed > block->allocation_size)
        new_committed = block->allocation_size;
    if (!lor_virtual__commit_raw(block_base + block->committed,
                                 new_committed - block->committed))
        return 0;

    block->committed = new_committed;
    return 1;
}

static void *lor_arena__block_alloc(LorArena *arena, LorArenaBlock *block,
                                    size_t size, size_t alignment) {
    unsigned char *base = lor_arena__block_data(block);
    if (base == NULL || block->used > block->capacity) return NULL;

    uintptr_t current = (uintptr_t)(base + block->used);
    uintptr_t aligned = 0;
    if (!lor_memory__align_forward(current, alignment, &aligned)) return NULL;

    size_t padding = (size_t)(aligned - current);
    if (padding > block->capacity - block->used) return NULL;
    if (size > block->capacity - block->used - padding) return NULL;

    size_t new_used = block->used + padding + size;
    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL &&
        !lor_arena__block_ensure_committed(block, new_used, arena->commit_size))
        return NULL;

    block->used = new_used;
    return (void *)aligned;
}

static LorArenaBlock *lor_arena__block_new(LorArena *arena, size_t min_capacity) {
    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL)
        return lor_arena__virtual_block_new(arena->reserve_size, arena->commit_size,
                                            min_capacity);

    if (arena->block_size < min_capacity)
        return lor_arena__heap_block_new(min_capacity);
    return lor_arena__heap_block_new(arena->block_size);
}

static int lor_arena__init_internal(LorArena *arena, LorArenaConfig config,
                                    int track_leakcheck, const char *file,
                                    int line) {
#if !defined(LOR_LEAKCHECK)
    (void)file;
    (void)line;
#endif
    if (arena == NULL || arena->blocks != NULL ||
        arena->backend != LOR_ARENA_BACKEND_HEAP || arena->block_size != 0 ||
        arena->reserve_size != 0 || arena->commit_size != 0)
        return 0;

    arena->blocks = NULL;
    arena->backend = config.backend;
    arena->block_size =
        config.block_size != 0 ? config.block_size : LOR_ARENA_DEFAULT_BLOCK_SIZE;
    arena->reserve_size = config.reserve_size != 0 ? config.reserve_size
                                                   : LOR_ARENA_DEFAULT_RESERVE_SIZE;
    arena->commit_size =
        config.commit_size != 0 ? config.commit_size : LOR_ARENA_DEFAULT_COMMIT_SIZE;

    if (arena->backend != LOR_ARENA_BACKEND_HEAP &&
        arena->backend != LOR_ARENA_BACKEND_VIRTUAL) {
        *arena = (LorArena)LOR_ARENA_INIT;
        return 0;
    }

    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL) {
        arena->reserve_size = lor_virtual__page_align(arena->reserve_size);
        arena->commit_size = lor_virtual__page_align(arena->commit_size);
        if (arena->reserve_size == 0 || arena->commit_size == 0) {
            *arena = (LorArena)LOR_ARENA_INIT;
            return 0;
        }
    }

    if (track_leakcheck) lor_leak__track(LOR_LEAK_KIND_ARENA, arena, 0, file, line);

    return 1;
}

#if defined(LOR_LEAKCHECK)
int lor_arena_init_config_debug(LorArena *arena, LorArenaConfig config,
                                const char *file, int line) {
    return lor_arena__init_internal(arena, config, 1, file, line);
}
#endif

int lor_arena_init_config(LorArena *arena, LorArenaConfig config) {
    return lor_arena__init_internal(arena, config, 1, NULL, 0);
}

void lor_arena_deinit(LorArena *arena) {
    if (arena == NULL) return;

    LorArenaBlock *block = arena->blocks;
    while (block != NULL) {
        LorArenaBlock *next = block->next;
        lor_arena__block_free(arena, block);
        block = next;
    }

    lor_leak__untrack(LOR_LEAK_KIND_ARENA, arena);
    *arena = (LorArena)LOR_ARENA_INIT;
}

void lor_arena_reset(LorArena *arena) {
    if (arena == NULL) return;

    for (LorArenaBlock *block = arena->blocks; block != NULL; block = block->next)
        block->used = 0;
}

LorArenaMark lor_arena_mark(const LorArena *arena) {
    if (arena == NULL) return (LorArenaMark)LOR_ARENA_MARK_INIT;

    return (LorArenaMark){
        .block = arena->blocks,
        .used = arena->blocks != NULL ? arena->blocks->used : 0,
    };
}

static int lor_arena__has_block(const LorArena *arena, const LorArenaBlock *target) {
    if (arena == NULL || target == NULL) return 0;

    for (const LorArenaBlock *block = arena->blocks; block != NULL;
         block = block->next)
        if (block == target) return 1;

    return 0;
}

void lor_arena_rewind(LorArena *arena, LorArenaMark mark) {
    if (arena == NULL) return;

    if (mark.block == NULL) {
        for (LorArenaBlock *block = arena->blocks; block != NULL;
             block = block->next)
            block->used = 0;
        return;
    }

    if (!lor_arena__has_block(arena, mark.block)) {
        assert(!"lor_arena_rewind mark does not belong to arena");
        return;
    }

    while (arena->blocks != NULL && arena->blocks != mark.block) {
        LorArenaBlock *block = arena->blocks;
        arena->blocks = block->next;
        lor_arena__block_free(arena, block);
    }

    if (mark.used > arena->blocks->used) {
        assert(!"lor_arena_rewind mark used exceeds block used");
        return;
    }

    arena->blocks->used = mark.used;
}

static void *lor_arena__alloc_aligned(LorArena *arena, size_t size, size_t alignment,
                                      const char *file, int line) {
#if !defined(LOR_LEAKCHECK)
    (void)file;
    (void)line;
#endif

    if (arena == NULL || size == 0 || !lor_memory__is_power_of_two(alignment))
        return NULL;

    if (arena->block_size == 0 && arena->reserve_size == 0)
        if (!lor_arena__init_internal(arena, (LorArenaConfig){0}, 1, file, line))
            return NULL;

    if (arena->blocks != NULL) {
        void *result = lor_arena__block_alloc(arena, arena->blocks, size, alignment);
        if (result != NULL) return result;
    }

    size_t min_capacity = size;
    if (alignment > 1u) {
        if (lor_memory__add_overflows_size(min_capacity, alignment - 1u))
            return NULL;
        min_capacity += alignment - 1u;
    }

    LorArenaBlock *block = lor_arena__block_new(arena, min_capacity);
    if (block == NULL) return NULL;

    block->next = arena->blocks;
    arena->blocks = block;
    return lor_arena__block_alloc(arena, block, size, alignment);
}

void *lor_arena_alloc(LorArena *arena, size_t size) {
    return lor_arena__alloc_aligned(arena, size, LOR_ARENA_MAX_ALIGNMENT, NULL, 0);
}

void *lor_arena_alloc_zero(LorArena *arena, size_t size) {
    void *ptr =
        lor_arena__alloc_aligned(arena, size, LOR_ARENA_MAX_ALIGNMENT, NULL, 0);

    if (ptr != NULL) memset(ptr, 0, size);
    return ptr;
}

#if defined(LOR_LEAKCHECK)
void *lor_arena_alloc_debug(LorArena *arena, size_t size, const char *file,
                            int line) {
    return lor_arena__alloc_aligned(arena, size, LOR_ARENA_MAX_ALIGNMENT, file,
                                    line);
}

void *lor_arena_alloc_zero_debug(LorArena *arena, size_t size, const char *file,
                                 int line) {
    void *ptr =
        lor_arena__alloc_aligned(arena, size, LOR_ARENA_MAX_ALIGNMENT, file, line);

    if (ptr != NULL) memset(ptr, 0, size);
    return ptr;
}
#endif

void *lor_arena_alloc_array(LorArena *arena, size_t count, size_t elem_size) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size))
        return NULL;

    return lor_arena_alloc(arena, count * elem_size);
}

void *lor_arena_alloc_array_zero(LorArena *arena, size_t count, size_t elem_size) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size))
        return NULL;

    return lor_arena_alloc_zero(arena, count * elem_size);
}

#if defined(LOR_LEAKCHECK)
void *lor_arena_alloc_array_debug(LorArena *arena, size_t count, size_t elem_size,
                                  const char *file, int line) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size))
        return NULL;

    return lor_arena_alloc_debug(arena, count * elem_size, file, line);
}

void *lor_arena_alloc_array_zero_debug(LorArena *arena, size_t count,
                                       size_t elem_size, const char *file,
                                       int line) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size))
        return NULL;

    return lor_arena_alloc_zero_debug(arena, count * elem_size, file, line);
}
#endif

static char *lor_arena__strdup_at(LorArena *arena, const char *text,
                                  const char *file, int line) {
    if (text == NULL) return NULL;

    size_t len = strlen(text);
    if (len == SIZE_MAX) return NULL;

    char *copy = (char *)lor_arena__alloc_aligned(
        arena, len + 1u, LOR_ARENA_MAX_ALIGNMENT, file, line);
    if (copy == NULL) return NULL;

    memcpy(copy, text, len + 1u);
    return copy;
}

char *lor_arena_strdup(LorArena *arena, const char *text) {
    return lor_arena__strdup_at(arena, text, NULL, 0);
}

#if defined(LOR_LEAKCHECK)
char *lor_arena_strdup_debug(LorArena *arena, const char *text, const char *file,
                             int line) {
    return lor_arena__strdup_at(arena, text, file, line);
}
#endif

size_t lor_arena_used(const LorArena *arena) {
    if (arena == NULL) return 0;

    size_t total = 0;
    for (const LorArenaBlock *block = arena->blocks; block != NULL;
         block = block->next)
        total += block->used;
    return total;
}

size_t lor_arena_capacity(const LorArena *arena) {
    if (arena == NULL) return 0;

    size_t total = 0;
    for (const LorArenaBlock *block = arena->blocks; block != NULL;
         block = block->next)
        total += block->capacity;
    return total;
}

size_t lor_arena_committed(const LorArena *arena) {
    if (arena == NULL) return 0;

    size_t total = 0;
    for (const LorArenaBlock *block = arena->blocks; block != NULL;
         block = block->next) {
        if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL) {
            unsigned char *base = (unsigned char *)block;
            unsigned char *data = lor_arena__block_data((LorArenaBlock *)block);
            size_t data_offset = data != NULL ? (size_t)(data - base) : 0;
            if (block->committed > data_offset)
                total += block->committed - data_offset;
        } else
            total += block->committed;
    }
    return total;
}

LorScratch lor_scratch_begin(const LorArena *conflict) {
    for (size_t i = 0; i < 2u; ++i) {
        if (conflict == &lor_memory__scratch_arenas[i]) continue;

        if (!lor_memory__scratch_inited[i]) {
            LorArenaConfig config = {
                .backend = LOR_ARENA_BACKEND_HEAP,
                .block_size = LOR_ARENA_DEFAULT_BLOCK_SIZE,
            };
            if (!lor_arena__init_internal(&lor_memory__scratch_arenas[i], config, 0,
                                          NULL, 0))
                return (LorScratch)LOR_SCRATCH_INIT;
            lor_memory__scratch_inited[i] = true;
        }

        return (LorScratch){
            .arena = &lor_memory__scratch_arenas[i],
            .mark = lor_arena_mark(&lor_memory__scratch_arenas[i]),
        };
    }

    return (LorScratch)LOR_SCRATCH_INIT;
}

void lor_scratch_end(LorScratch scratch) {
    if (scratch.arena == NULL) return;
    lor_arena_rewind(scratch.arena, scratch.mark);
}

void lor_scratch_cleanup(void) {
    for (size_t i = 0; i < 2u; ++i) {
        if (lor_memory__scratch_inited[i]) {
            lor_arena_deinit(&lor_memory__scratch_arenas[i]);
            lor_memory__scratch_inited[i] = false;
        }
    }
}

#if defined(_WIN32)
static DWORD lor_mmap__windows_protect(LorMmapMode mode) {
    switch (mode) {
    case LOR_MMAP_SHARED:
        return PAGE_READWRITE;
    case LOR_MMAP_COPY:
        return PAGE_WRITECOPY;
    case LOR_MMAP_READ:
        return PAGE_READONLY;
    }
    return PAGE_READONLY;
}

static DWORD lor_mmap__windows_access(LorMmapMode mode) {
    switch (mode) {
    case LOR_MMAP_SHARED:
        return FILE_MAP_WRITE;
    case LOR_MMAP_COPY:
        return FILE_MAP_COPY;
    case LOR_MMAP_READ:
        return FILE_MAP_READ;
    }
    return FILE_MAP_READ;
}
#endif

static int lor_mmap__mode_valid(LorMmapMode mode) {
    return mode == LOR_MMAP_READ || mode == LOR_MMAP_COPY ||
           mode == LOR_MMAP_SHARED;
}

static LorMmap lor_mmap__file_at(const char *path, LorMmapMode mode,
                                 const char *file, int line) {
    LorMmap map = {0};
#if !defined(LOR_LEAKCHECK)
    (void)file;
    (void)line;
#endif
    if (path == NULL || !lor_mmap__mode_valid(mode)) return map;

#if defined(_WIN32)
    {
        DWORD access =
            mode == LOR_MMAP_SHARED ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
        DWORD create = OPEN_EXISTING;
        LARGE_INTEGER file_size;
        HANDLE file_handle =
            CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        create, FILE_ATTRIBUTE_NORMAL, NULL);

        if (file_handle == INVALID_HANDLE_VALUE) return map;
        if (!GetFileSizeEx(file_handle, &file_size) || file_size.QuadPart <= 0) {
            CloseHandle(file_handle);
            return map;
        }
        if ((uint64_t)file_size.QuadPart > SIZE_MAX) {
            CloseHandle(file_handle);
            return map;
        }

        HANDLE mapping = CreateFileMappingA(
            file_handle, NULL, lor_mmap__windows_protect(mode), 0, 0, NULL);
        CloseHandle(file_handle);
        if (mapping == NULL) return map;

        map.data = MapViewOfFile(mapping, lor_mmap__windows_access(mode), 0, 0, 0);
        CloseHandle(mapping);
        if (map.data == NULL) return (LorMmap){0};

        map.size = (size_t)file_size.QuadPart;
    }
#else
    {
        int flags = mode == LOR_MMAP_SHARED ? O_RDWR : O_RDONLY;
        int prot = PROT_READ;
        int mmap_flags = mode == LOR_MMAP_SHARED ? MAP_SHARED : MAP_PRIVATE;
        struct stat st;
        int fd = open(path, flags);

        if (fd < 0) return map;
        if (fstat(fd, &st) != 0 || st.st_size <= 0) {
            close(fd);
            return map;
        }
        if ((uint64_t)st.st_size > SIZE_MAX) {
            close(fd);
            return map;
        }

        if (mode != LOR_MMAP_READ) prot |= PROT_WRITE;

        map.size = (size_t)st.st_size;
        map.data = mmap(NULL, map.size, prot, mmap_flags, fd, 0);
        close(fd);
        if (map.data == MAP_FAILED) return (LorMmap){0};
    }
#endif

    lor_leak__track(LOR_LEAK_KIND_MMAP, map.data, map.size, file, line);
    return map;
}

#if defined(LOR_LEAKCHECK)
LorMmap lor_mmap_file_debug(const char *path, LorMmapMode mode, const char *file,
                            int line) {
    return lor_mmap__file_at(path, mode, file, line);
}
#endif

LorMmap lor_mmap_file(const char *path, LorMmapMode mode) {
    return lor_mmap__file_at(path, mode, NULL, 0);
}

void lor_mmap_unmap(LorMmap *map) {
    if (map == NULL || map->data == NULL) return;

    lor_leak__untrack(LOR_LEAK_KIND_MMAP, map->data);

#if defined(_WIN32)
    (void)UnmapViewOfFile(map->data);
#else
    (void)munmap(map->data, map->size);
#endif

    map->data = NULL;
    map->size = 0;
}
