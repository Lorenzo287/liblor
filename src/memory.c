// SPDX-License-Identifier: MIT

#include "lor/memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define LOR_ARENA_MAX_ALIGNMENT alignof(max_align_t)
#else
#define LOR_ARENA_MAX_ALIGNMENT \
    (sizeof(void *) > sizeof(double) ? sizeof(void *) : sizeof(double))
#endif

struct LorArenaBlock {
    struct LorArenaBlock *next;
    size_t capacity;
    size_t used;
    unsigned char data[];
};

static int lor_memory__mul_overflows_size(size_t a, size_t b) {
    return b != 0 && a > SIZE_MAX / b;
}

static int lor_arena__is_power_of_two(size_t value) {
    return value != 0 && (value & (value - 1u)) == 0;
}

static int lor_arena__add_overflows_size(size_t a, size_t b) {
    return a > SIZE_MAX - b;
}

static int lor_arena__align_forward(uintptr_t value, size_t alignment,
                                    uintptr_t *out) {
    uintptr_t mask = (uintptr_t)(alignment - 1u);

    if (value > UINTPTR_MAX - mask) { return 0; }

    *out = (value + mask) & ~mask;
    return 1;
}

static void *lor_allocator__heap_realloc(void *ctx, void *ptr, size_t old_size,
                                         size_t new_size) {
    (void)ctx;
    (void)old_size;

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, new_size);
}

static unsigned char *lor_arena__block_data(LorArenaBlock *block) {
    uintptr_t aligned = 0;

    if (!lor_arena__align_forward((uintptr_t)block->data, LOR_ARENA_MAX_ALIGNMENT,
                                  &aligned)) {
        return NULL;
    }

    return (unsigned char *)aligned;
}

static LorArenaBlock *lor_arena__block_new(size_t capacity) {
    size_t header_size = offsetof(LorArenaBlock, data);
    size_t extra = LOR_ARENA_MAX_ALIGNMENT - 1u;

    if (lor_arena__add_overflows_size(capacity, extra) ||
        lor_arena__add_overflows_size(header_size, capacity + extra)) {
        return NULL;
    }

    LorArenaBlock *block = (LorArenaBlock *)malloc(header_size + capacity + extra);
    if (block == NULL) { return NULL; }

    block->next = NULL;
    block->capacity = capacity;
    block->used = 0;
    return block;
}

static void *lor_arena__block_alloc(LorArenaBlock *block, size_t size,
                                    size_t alignment) {
    unsigned char *base = lor_arena__block_data(block);
    uintptr_t current = 0;
    uintptr_t aligned = 0;
    size_t padding = 0;

    if (base == NULL || block->used > block->capacity) { return NULL; }

    current = (uintptr_t)(base + block->used);
    if (!lor_arena__align_forward(current, alignment, &aligned)) { return NULL; }

    padding = (size_t)(aligned - current);
    if (padding > block->capacity - block->used) { return NULL; }

    if (size > block->capacity - block->used - padding) { return NULL; }

    block->used += padding + size;
    return (void *)aligned;
}

static size_t lor_arena__defaulted_block_size(size_t block_size) {
    return block_size != 0 ? block_size : LOR_ARENA_DEFAULT_BLOCK_SIZE;
}

LorAllocator lor_allocator_heap(void) {
    LorAllocator allocator = {NULL, lor_allocator__heap_realloc};
    return allocator;
}

int lor_allocator_is_valid(LorAllocator allocator) {
    return allocator.realloc != NULL;
}

void *lor_allocator_realloc(LorAllocator allocator, void *ptr, size_t old_size,
                            size_t new_size) {
    if (new_size == 0) {
        if (allocator.realloc != NULL) {
            allocator.realloc(allocator.ctx, ptr, old_size, 0);
        }
        return NULL;
    }

    if (allocator.realloc == NULL) return NULL;
    return allocator.realloc(allocator.ctx, ptr, old_size, new_size);
}

void *lor_allocator_alloc(LorAllocator allocator, size_t size) {
    return lor_allocator_realloc(allocator, NULL, 0, size);
}

void *lor_allocator_alloc_zero(LorAllocator allocator, size_t size) {
    void *ptr = lor_allocator_alloc(allocator, size);
    if (ptr != NULL) memset(ptr, 0, size);
    return ptr;
}

void lor_allocator_free(LorAllocator allocator, void *ptr, size_t old_size) {
    (void)lor_allocator_realloc(allocator, ptr, old_size, 0);
}

void *lor_allocator_alloc_array(LorAllocator allocator, size_t count,
                                size_t elem_size) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size)) {
        return NULL;
    }

    return lor_allocator_alloc(allocator, count * elem_size);
}

void *lor_allocator_alloc_array_zero(LorAllocator allocator, size_t count,
                                     size_t elem_size) {
    if (count == 0 || elem_size == 0 ||
        lor_memory__mul_overflows_size(count, elem_size)) {
        return NULL;
    }

    return lor_allocator_alloc_zero(allocator, count * elem_size);
}

void *lor_allocator_realloc_array(LorAllocator allocator, void *ptr,
                                  size_t old_count, size_t new_count,
                                  size_t elem_size) {
    if (elem_size == 0 || lor_memory__mul_overflows_size(old_count, elem_size) ||
        lor_memory__mul_overflows_size(new_count, elem_size)) {
        return NULL;
    }

    return lor_allocator_realloc(allocator, ptr, old_count * elem_size,
                                 new_count * elem_size);
}

void lor_arena_init(LorArena *arena, size_t block_size) {
    if (arena == NULL) { return; }

    arena->blocks = NULL;
    arena->block_size = lor_arena__defaulted_block_size(block_size);
}

void lor_arena_deinit(LorArena *arena) {
    LorArenaBlock *block = NULL;

    if (arena == NULL) { return; }

    block = arena->blocks;
    while (block != NULL) {
        LorArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    arena->blocks = NULL;
    arena->block_size = 0;
}

void lor_arena_reset(LorArena *arena) {
    LorArenaBlock *block = NULL;

    if (arena == NULL) { return; }

    for (block = arena->blocks; block != NULL; block = block->next) {
        block->used = 0;
    }
}

void *lor_arena_alloc_aligned(LorArena *arena, size_t size, size_t alignment) {
    size_t block_capacity = 0;
    size_t min_capacity = 0;
    LorArenaBlock *block = NULL;
    void *result = NULL;

    if (arena == NULL || size == 0 || !lor_arena__is_power_of_two(alignment)) {
        return NULL;
    }

    arena->block_size = lor_arena__defaulted_block_size(arena->block_size);

    if (arena->blocks != NULL) {
        result = lor_arena__block_alloc(arena->blocks, size, alignment);
        if (result != NULL) { return result; }
    }

    min_capacity = size;
    if (alignment > 1u) {
        if (lor_arena__add_overflows_size(min_capacity, alignment - 1u)) {
            return NULL;
        }
        min_capacity += alignment - 1u;
    }

    block_capacity = arena->block_size;
    if (block_capacity < min_capacity) { block_capacity = min_capacity; }

    block = lor_arena__block_new(block_capacity);
    if (block == NULL) { return NULL; }

    block->next = arena->blocks;
    arena->blocks = block;
    return lor_arena__block_alloc(block, size, alignment);
}

void *lor_arena_alloc(LorArena *arena, size_t size) {
    return lor_arena_alloc_aligned(arena, size, LOR_ARENA_MAX_ALIGNMENT);
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
