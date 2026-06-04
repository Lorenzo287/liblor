// SPDX-License-Identifier: MIT

#define LOR_MEMORY_NO_STDLIB_MACROS
#define LOR_MEMORY_NO_LOCATION_MACROS
#define LOR_MEMORY_INTERNAL
#include "lor/memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    LorArenaBlock *next;
    size_t capacity;
    size_t committed;
    size_t used;
    size_t allocation_size;
    unsigned char data[];
};

struct LorArenaScope {
    LorArenaScope *prev;
    LorArenaBlock *block;
    size_t used;
};

#if defined(LOR_LEAKCHECK)
static LorLeakRecord *lor_memory__leaks = NULL;
#endif
static LOR_THREAD_LOCAL LorArena lor_memory__scratch_arenas[2];
static LOR_THREAD_LOCAL int lor_memory__scratch_inited[2];

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

    if (value > UINTPTR_MAX - mask) { return 0; }

    *out = (value + mask) & ~mask;
    return 1;
}

#if defined(LOR_LEAKCHECK)
static void lor_leak__insert(LorLeakRecord *record) {
    record->prev = NULL;
    record->next = lor_memory__leaks;
    if (lor_memory__leaks != NULL) { lor_memory__leaks->prev = record; }
    lor_memory__leaks = record;
}

static void lor_leak__remove(LorLeakRecord *record) {
    if (record->prev != NULL) {
        record->prev->next = record->next;
    } else {
        lor_memory__leaks = record->next;
    }

    if (record->next != NULL) { record->next->prev = record->prev; }

    lor_memory__raw_free(record);
}

static LorLeakRecord *lor_leak__find(void *ptr, LorLeakKind kind) {
    LorLeakRecord *record = NULL;

    for (record = lor_memory__leaks; record != NULL; record = record->next) {
        if (record->ptr == ptr && record->kind == kind) { return record; }
    }

    return NULL;
}

static void lor_leak__track(LorLeakKind kind, void *ptr, size_t size,
                            const char *file, int line) {
    LorLeakRecord *record = NULL;

    if (ptr == NULL) { return; }

    record = (LorLeakRecord *)lor_memory__raw_malloc(sizeof(*record));
    if (record == NULL) { return; }

    record->kind = kind;
    record->ptr = ptr;
    record->size = size;
    record->file = file;
    record->line = line;
    lor_leak__insert(record);
}

static void lor_leak__untrack(LorLeakKind kind, void *ptr) {
    LorLeakRecord *record = lor_leak__find(ptr, kind);
    if (record != NULL) { lor_leak__remove(record); }
}

static const char *lor_leak__kind_name(LorLeakKind kind) {
    switch (kind) {
    case LOR_LEAK_KIND_HEAP: return "heap";
    case LOR_LEAK_KIND_ARENA: return "arena";
    case LOR_LEAK_KIND_MMAP: return "mmap";
    }

    return "unknown";
}
#else
#define lor_leak__track(kind, ptr, size, file, line) ((void)0)
#define lor_leak__untrack(kind, ptr) ((void)0)
#endif

LorLeakStats lor_leakcheck_stats(void) {
    LorLeakStats stats = {0};
#if defined(LOR_LEAKCHECK)
    LorLeakRecord *record = NULL;

    for (record = lor_memory__leaks; record != NULL; record = record->next) {
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
    LorLeakRecord *record = NULL;

    if (out == NULL) { out = stderr; }

    for (record = lor_memory__leaks; record != NULL; record = record->next) {
        const char *file = record->file != NULL ? record->file : "?";
        fprintf(out, "LEAK %s: %zu bytes at %p (%s:%d)\n",
                lor_leak__kind_name(record->kind), record->size, record->ptr,
                file, record->line);
        count += 1u;
    }
#else
    (void)out;
#endif

    return count;
}

#if defined(LOR_LEAKCHECK)
void *lor_malloc_debug(size_t size, const char *file, int line) {
    void *ptr = NULL;

    if (size == 0) { return NULL; }

    ptr = lor_memory__raw_malloc(size);
    lor_leak__track(LOR_LEAK_KIND_HEAP, ptr, size, file, line);
    return ptr;
}

void *lor_calloc_debug(size_t count, size_t elem_size, const char *file,
                       int line) {
    void *ptr = NULL;

    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size)) {
        return NULL;
    }

    ptr = lor_memory__raw_calloc(count, elem_size);
    lor_leak__track(LOR_LEAK_KIND_HEAP, ptr, count * elem_size, file, line);
    return ptr;
}

void lor_free_debug(void *ptr, const char *file, int line) {
    (void)file;
    (void)line;

    if (ptr == NULL) { return; }

    lor_leak__untrack(LOR_LEAK_KIND_HEAP, ptr);
    lor_memory__raw_free(ptr);
}

void *lor_realloc_debug(void *ptr, size_t size, const char *file, int line) {
    LorLeakRecord *record = NULL;
    void *new_ptr = NULL;

    if (ptr == NULL) { return lor_malloc_debug(size, file, line); }

    if (size == 0) {
        lor_free_debug(ptr, file, line);
        return NULL;
    }

    record = lor_leak__find(ptr, LOR_LEAK_KIND_HEAP);
    new_ptr = lor_memory__raw_realloc(ptr, size);
    if (new_ptr == NULL) { return NULL; }

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
    size_t len = 0;
    char *copy = NULL;

    if (text == NULL) { return NULL; }

    len = strlen(text);
    if (len == SIZE_MAX) { return NULL; }

    copy = (char *)lor_malloc_debug(len + 1u, file, line);
    if (copy == NULL) { return NULL; }

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
    if (page_size <= 0) { return LOR_KIB(4); }
    return (size_t)page_size;
#endif
}

static size_t lor_virtual__page_align(size_t size) {
    size_t page_size = lor_page_size();
    if (page_size == 0) { return size; }
    if (lor_memory__add_overflows_size(size, page_size - 1u)) { return 0; }
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

static size_t lor_arena__defaulted_block_size(size_t block_size) {
    return block_size != 0 ? block_size : LOR_ARENA_DEFAULT_BLOCK_SIZE;
}

static size_t lor_arena__defaulted_reserve_size(size_t reserve_size) {
    return reserve_size != 0 ? reserve_size : LOR_ARENA_DEFAULT_RESERVE_SIZE;
}

static size_t lor_arena__defaulted_commit_size(size_t commit_size) {
    return commit_size != 0 ? commit_size : LOR_ARENA_DEFAULT_COMMIT_SIZE;
}

static unsigned char *lor_arena__block_data(LorArenaBlock *block) {
    uintptr_t aligned = 0;

    if (!lor_memory__align_forward((uintptr_t)block->data,
                                   LOR_ARENA_MAX_ALIGNMENT, &aligned)) {
        return NULL;
    }

    return (unsigned char *)aligned;
}

static size_t lor_arena__block_header_slack(void) {
    return offsetof(LorArenaBlock, data) + LOR_ARENA_MAX_ALIGNMENT - 1u;
}

static LorArenaBlock *lor_arena__heap_block_new(size_t capacity) {
    size_t allocation_size = 0;
    LorArenaBlock *block = NULL;
    size_t slack = lor_arena__block_header_slack();

    if (lor_memory__add_overflows_size(capacity, slack)) { return NULL; }

    allocation_size = capacity + slack;
    block = (LorArenaBlock *)lor_memory__raw_malloc(allocation_size);
    if (block == NULL) { return NULL; }

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
    size_t min_reserve = 0;
    size_t data_offset = 0;
    LorArenaBlock *block = NULL;
    unsigned char *base = NULL;
    unsigned char *data = NULL;

    if (lor_memory__add_overflows_size(min_capacity, slack)) { return NULL; }

    min_reserve = min_capacity + slack;
    if (reserve_size < min_reserve) { reserve_size = min_reserve; }
    reserve_size = lor_virtual__page_align(reserve_size);
    if (reserve_size == 0) { return NULL; }

    if (commit_size < slack) { commit_size = slack; }
    if (commit_size < min_reserve && min_reserve < LOR_ARENA_DEFAULT_COMMIT_SIZE) {
        commit_size = min_reserve;
    }
    commit_size = lor_virtual__page_align(commit_size);
    if (commit_size > reserve_size) { commit_size = reserve_size; }

    block = (LorArenaBlock *)lor_virtual__reserve_raw(reserve_size);
    if (block == NULL) { return NULL; }
    if (commit_size != 0 && !lor_virtual__commit_raw(block, commit_size)) {
        lor_virtual__release_raw(block, reserve_size);
        return NULL;
    }

    block->next = NULL;
    block->capacity = 0;
    block->committed = commit_size;
    block->used = 0;
    block->allocation_size = reserve_size;

    base = (unsigned char *)block;
    data = lor_arena__block_data(block);
    if (data == NULL) {
        lor_virtual__release_raw(block, reserve_size);
        return NULL;
    }

    data_offset = (size_t)(data - base);
    block->capacity = reserve_size - data_offset;
    return block;
}

static void lor_arena__block_free(LorArena *arena, LorArenaBlock *block) {
    if (block == NULL) { return; }

    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL) {
        lor_virtual__release_raw(block, block->allocation_size);
    } else {
        lor_memory__raw_free(block);
    }
}

static int lor_arena__block_ensure_committed(LorArenaBlock *block,
                                             size_t needed_used,
                                             size_t commit_size) {
    unsigned char *base = lor_arena__block_data(block);
    unsigned char *block_base = (unsigned char *)block;
    size_t data_offset = 0;
    size_t needed_total = 0;
    size_t new_committed = 0;

    if (base == NULL || needed_used > block->capacity) { return 0; }

    data_offset = (size_t)(base - block_base);
    if (lor_memory__add_overflows_size(data_offset, needed_used)) { return 0; }
    needed_total = data_offset + needed_used;

    if (needed_total <= block->committed) { return 1; }

    new_committed = needed_total;
    if (commit_size != 0) {
        if (lor_memory__add_overflows_size(new_committed, commit_size - 1u)) {
            return 0;
        }
        new_committed = lor_memory__align_up(new_committed, commit_size);
    }

    if (new_committed > block->allocation_size) {
        new_committed = block->allocation_size;
    }
    if (!lor_virtual__commit_raw(block_base + block->committed,
                                 new_committed - block->committed)) {
        return 0;
    }

    block->committed = new_committed;
    return 1;
}

static void *lor_arena__block_alloc(LorArena *arena, LorArenaBlock *block,
                                    size_t size, size_t alignment) {
    unsigned char *base = lor_arena__block_data(block);
    uintptr_t current = 0;
    uintptr_t aligned = 0;
    size_t padding = 0;
    size_t new_used = 0;

    if (base == NULL || block->used > block->capacity) { return NULL; }

    current = (uintptr_t)(base + block->used);
    if (!lor_memory__align_forward(current, alignment, &aligned)) { return NULL; }

    padding = (size_t)(aligned - current);
    if (padding > block->capacity - block->used) { return NULL; }
    if (size > block->capacity - block->used - padding) { return NULL; }

    new_used = block->used + padding + size;
    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL &&
        !lor_arena__block_ensure_committed(block, new_used, arena->commit_size)) {
        return NULL;
    }

    block->used = new_used;
    return (void *)aligned;
}

static LorArenaBlock *lor_arena__block_new(LorArena *arena, size_t min_capacity) {
    if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL) {
        return lor_arena__virtual_block_new(arena->reserve_size, arena->commit_size,
                                            min_capacity);
    }

    if (arena->block_size < min_capacity) { return lor_arena__heap_block_new(min_capacity); }
    return lor_arena__heap_block_new(arena->block_size);
}

static int lor_arena__init_internal(LorArena *arena, const LorArenaConfig *config,
                                    int track_leakcheck, const char *file,
                                    int line) {
    LorArenaConfig defaults = {0};
#if !defined(LOR_LEAKCHECK)
    (void)file;
    (void)line;
#endif

    if (arena == NULL) { return 0; }

    if (config == NULL) { config = &defaults; }

    arena->blocks = NULL;
    arena->scopes = NULL;
    arena->backend = config->backend;
    arena->block_size = lor_arena__defaulted_block_size(config->block_size);
    arena->reserve_size = lor_arena__defaulted_reserve_size(config->reserve_size);
    arena->commit_size = lor_arena__defaulted_commit_size(config->commit_size);

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

    if (track_leakcheck) { lor_leak__track(LOR_LEAK_KIND_ARENA, arena, 0, file, line); }

    return 1;
}

static void lor_arena__init_at(LorArena *arena, size_t block_size,
                               const char *file, int line) {
    LorArenaConfig config = {0};
    config.backend = LOR_ARENA_BACKEND_HEAP;
    config.block_size = block_size;
    (void)lor_arena__init_internal(arena, &config, 1, file, line);
}

#if defined(LOR_LEAKCHECK)
void lor_arena_init_debug(LorArena *arena, size_t block_size, const char *file,
                          int line) {
    lor_arena__init_at(arena, block_size, file, line);
}
#endif

void lor_arena_init(LorArena *arena, size_t block_size) {
    lor_arena__init_at(arena, block_size, NULL, 0);
}

#if defined(LOR_LEAKCHECK)
int lor_arena_init_ex_debug(LorArena *arena, const LorArenaConfig *config,
                            const char *file, int line) {
    return lor_arena__init_internal(arena, config, 1, file, line);
}
#endif

int lor_arena_init_ex(LorArena *arena, const LorArenaConfig *config) {
    return lor_arena__init_internal(arena, config, 1, NULL, 0);
}

void lor_arena_deinit(LorArena *arena) {
    LorArenaBlock *block = NULL;
    LorArenaScope *scope = NULL;

    if (arena == NULL) { return; }

    scope = arena->scopes;
    while (scope != NULL) {
        LorArenaScope *prev = scope->prev;
        lor_memory__raw_free(scope);
        scope = prev;
    }

    block = arena->blocks;
    while (block != NULL) {
        LorArenaBlock *next = block->next;
        lor_arena__block_free(arena, block);
        block = next;
    }

    lor_leak__untrack(LOR_LEAK_KIND_ARENA, arena);

    *arena = (LorArena)LOR_ARENA_INIT;
}

void lor_arena_reset(LorArena *arena) {
    LorArenaBlock *block = NULL;
    LorArenaScope *scope = NULL;

    if (arena == NULL) { return; }

    scope = arena->scopes;
    while (scope != NULL) {
        LorArenaScope *prev = scope->prev;
        lor_memory__raw_free(scope);
        scope = prev;
    }
    arena->scopes = NULL;

    for (block = arena->blocks; block != NULL; block = block->next) {
        block->used = 0;
    }
}

int lor_arena_mark(LorArena *arena) {
    LorArenaScope *scope = NULL;

    if (arena == NULL) { return 0; }

    scope = (LorArenaScope *)lor_memory__raw_malloc(sizeof(*scope));
    if (scope == NULL) { return 0; }

    scope->block = arena->blocks;
    scope->used = arena->blocks != NULL ? arena->blocks->used : 0;
    scope->prev = arena->scopes;
    arena->scopes = scope;
    return 1;
}

static void lor_arena__rewind_to(LorArena *arena, LorArenaBlock *mark_block,
                                 size_t mark_used) {
    LorArenaBlock *block = NULL;

    if (arena == NULL) { return; }

    if (mark_block == NULL) {
        for (block = arena->blocks; block != NULL; block = block->next) {
            block->used = 0;
        }
        return;
    }

    while (arena->blocks != NULL && arena->blocks != mark_block) {
        block = arena->blocks;
        arena->blocks = block->next;
        lor_arena__block_free(arena, block);
    }

    if (arena->blocks != NULL) {
        if (mark_used <= arena->blocks->used) { arena->blocks->used = mark_used; }
    }
}

void lor_arena_rewind(LorArena *arena) {
    LorArenaScope *scope = NULL;

    if (arena == NULL || arena->scopes == NULL) { return; }

    scope = arena->scopes;
    arena->scopes = scope->prev;
    lor_arena__rewind_to(arena, scope->block, scope->used);
    lor_memory__raw_free(scope);
}

static void *lor_arena__alloc_aligned(LorArena *arena, size_t size,
                                      size_t alignment) {
    size_t min_capacity = 0;
    LorArenaBlock *block = NULL;
    void *result = NULL;

    if (arena == NULL || size == 0 || !lor_memory__is_power_of_two(alignment)) {
        return NULL;
    }

    if (arena->block_size == 0 && arena->reserve_size == 0) {
        lor_arena_init(arena, 0);
    }

    if (arena->blocks != NULL) {
        result = lor_arena__block_alloc(arena, arena->blocks, size, alignment);
        if (result != NULL) { return result; }
    }

    min_capacity = size;
    if (alignment > 1u) {
        if (lor_memory__add_overflows_size(min_capacity, alignment - 1u)) {
            return NULL;
        }
        min_capacity += alignment - 1u;
    }

    block = lor_arena__block_new(arena, min_capacity);
    if (block == NULL) { return NULL; }

    block->next = arena->blocks;
    arena->blocks = block;
    return lor_arena__block_alloc(arena, block, size, alignment);
}

void *lor_arena_alloc(LorArena *arena, size_t size) {
    return lor_arena__alloc_aligned(arena, size, LOR_ARENA_MAX_ALIGNMENT);
}

void *lor_arena_alloc_zero(LorArena *arena, size_t size) {
    void *ptr = lor_arena_alloc(arena, size);

    if (ptr != NULL) { memset(ptr, 0, size); }

    return ptr;
}

void *lor_arena_alloc_array(LorArena *arena, size_t count, size_t elem_size) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size)) {
        return NULL;
    }

    return lor_arena_alloc(arena, count * elem_size);
}

void *lor_arena_alloc_array_zero(LorArena *arena, size_t count, size_t elem_size) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size)) {
        return NULL;
    }

    return lor_arena_alloc_zero(arena, count * elem_size);
}

char *lor_arena_strdup(LorArena *arena, const char *text) {
    size_t len = 0;
    char *copy = NULL;

    if (text == NULL) { return NULL; }

    len = strlen(text);
    if (len == SIZE_MAX) { return NULL; }

    copy = (char *)lor_arena_alloc(arena, len + 1u);
    if (copy == NULL) { return NULL; }

    memcpy(copy, text, len + 1u);
    return copy;
}

size_t lor_arena_used(const LorArena *arena) {
    size_t total = 0;
    const LorArenaBlock *block = NULL;

    if (arena == NULL) { return 0; }

    for (block = arena->blocks; block != NULL; block = block->next) {
        total += block->used;
    }

    return total;
}

size_t lor_arena_capacity(const LorArena *arena) {
    size_t total = 0;
    const LorArenaBlock *block = NULL;

    if (arena == NULL) { return 0; }

    for (block = arena->blocks; block != NULL; block = block->next) {
        total += block->capacity;
    }

    return total;
}

size_t lor_arena_committed(const LorArena *arena) {
    size_t total = 0;
    const LorArenaBlock *block = NULL;

    if (arena == NULL) { return 0; }

    for (block = arena->blocks; block != NULL; block = block->next) {
        if (arena->backend == LOR_ARENA_BACKEND_VIRTUAL) {
            unsigned char *base = (unsigned char *)block;
            unsigned char *data = lor_arena__block_data((LorArenaBlock *)block);
            size_t data_offset = data != NULL ? (size_t)(data - base) : 0;
            if (block->committed > data_offset) {
                total += block->committed - data_offset;
            }
        } else {
            total += block->committed;
        }
    }

    return total;
}

LorScratch lor_scratch_begin(LorArena **conflicts, size_t conflict_count) {
    size_t i = 0;
    size_t j = 0;

    for (i = 0; i < 2u; ++i) {
        int conflict = 0;

        for (j = 0; j < conflict_count; ++j) {
            if (conflicts != NULL && conflicts[j] == &lor_memory__scratch_arenas[i]) {
                conflict = 1;
                break;
            }
        }

        if (conflict) { continue; }

        if (!lor_memory__scratch_inited[i]) {
            LorArenaConfig config = {0};
            config.backend = LOR_ARENA_BACKEND_HEAP;
            config.block_size = LOR_ARENA_DEFAULT_BLOCK_SIZE;
            if (!lor_arena__init_internal(&lor_memory__scratch_arenas[i], &config,
                                          0, NULL, 0)) {
                return (LorScratch){0};
            }
            lor_memory__scratch_inited[i] = 1;
        }

        return (LorScratch){
            &lor_memory__scratch_arenas[i],
            lor_memory__scratch_arenas[i].blocks,
            lor_memory__scratch_arenas[i].blocks != NULL
                ? lor_memory__scratch_arenas[i].blocks->used
                : 0,
        };
    }

    return (LorScratch){0};
}

void lor_scratch_end(LorScratch scratch) {
    lor_arena__rewind_to(scratch.arena, scratch.block, scratch.used);
}

void lor_scratch_cleanup_current_thread(void) {
    size_t i = 0;

    for (i = 0; i < 2u; ++i) {
        if (lor_memory__scratch_inited[i]) {
            lor_arena_deinit(&lor_memory__scratch_arenas[i]);
            lor_memory__scratch_inited[i] = 0;
        }
    }
}

#if defined(_WIN32)
static DWORD lor_mmap__windows_protect(LorMmapMode mode) {
    switch (mode) {
    case LOR_MMAP_SHARED: return PAGE_READWRITE;
    case LOR_MMAP_COPY: return PAGE_WRITECOPY;
    case LOR_MMAP_READ: return PAGE_READONLY;
    }
    return PAGE_READONLY;
}

static DWORD lor_mmap__windows_access(LorMmapMode mode) {
    switch (mode) {
    case LOR_MMAP_SHARED: return FILE_MAP_WRITE;
    case LOR_MMAP_COPY: return FILE_MAP_COPY;
    case LOR_MMAP_READ: return FILE_MAP_READ;
    }
    return FILE_MAP_READ;
}
#endif

static LorMmap lor_mmap__file_at(const char *path, LorMmapMode mode,
                                 const char *file, int line) {
    LorMmap map = {0};
#if !defined(LOR_LEAKCHECK)
    (void)file;
    (void)line;
#endif

    if (path == NULL) { return map; }

#if defined(_WIN32)
    {
        DWORD access =
            mode == LOR_MMAP_SHARED ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
        DWORD create = OPEN_EXISTING;
        LARGE_INTEGER file_size;
        HANDLE file = CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, create, FILE_ATTRIBUTE_NORMAL, NULL);
        HANDLE mapping = NULL;

        if (file == INVALID_HANDLE_VALUE) { return map; }
        if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart <= 0) {
            CloseHandle(file);
            return map;
        }
        if ((uint64_t)file_size.QuadPart > SIZE_MAX) {
            CloseHandle(file);
            return map;
        }

        mapping = CreateFileMappingA(file, NULL, lor_mmap__windows_protect(mode),
                                     0, 0, NULL);
        CloseHandle(file);
        if (mapping == NULL) { return map; }

        map.data = MapViewOfFile(mapping, lor_mmap__windows_access(mode), 0, 0, 0);
        CloseHandle(mapping);
        if (map.data == NULL) { return (LorMmap){0}; }

        map.size = (size_t)file_size.QuadPart;
    }
#else
    {
        int flags = mode == LOR_MMAP_SHARED ? O_RDWR : O_RDONLY;
        int prot = PROT_READ;
        int mmap_flags = mode == LOR_MMAP_SHARED ? MAP_SHARED : MAP_PRIVATE;
        struct stat st;
        int fd = open(path, flags);

        if (fd < 0) { return map; }
        if (fstat(fd, &st) != 0 || st.st_size <= 0) {
            close(fd);
            return map;
        }
        if ((uint64_t)st.st_size > SIZE_MAX) {
            close(fd);
            return map;
        }

        if (mode != LOR_MMAP_READ) { prot |= PROT_WRITE; }

        map.size = (size_t)st.st_size;
        map.data = mmap(NULL, map.size, prot, mmap_flags, fd, 0);
        close(fd);
        if (map.data == MAP_FAILED) { return (LorMmap){0}; }
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
    if (map == NULL || map->data == NULL) { return; }

    lor_leak__untrack(LOR_LEAK_KIND_MMAP, map->data);

#if defined(_WIN32)
    (void)UnmapViewOfFile(map->data);
#else
    (void)munmap(map->data, map->size);
#endif

    map->data = NULL;
    map->size = 0;
}
