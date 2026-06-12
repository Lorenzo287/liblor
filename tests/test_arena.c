// SPDX-License-Identifier: MIT

#include "lor/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define TEST_MAX_ALIGNMENT alignof(max_align_t)
#else
#define TEST_MAX_ALIGNMENT \
    (sizeof(void *) > sizeof(double) ? sizeof(void *) : sizeof(double))
#endif

#include "lor_test.h"

static int test_zero_initialized_arena(void) {
    LorArena arena = LOR_ARENA_INIT;
    int *value = (int *)lor_arena_alloc(&arena, sizeof(*value));

    CHECK(value != NULL);
    *value = 42;
    CHECK(*value == 42);
    CHECK(lor_arena_used(&arena) >= sizeof(*value));
    CHECK(lor_arena_capacity(&arena) >= LOR_ARENA_DEFAULT_BLOCK_SIZE);

    lor_arena_deinit(&arena);
    CHECK(lor_arena_used(&arena) == 0);
    CHECK(lor_arena_capacity(&arena) == 0);
    return 0;
}

static int test_reset_reuses_blocks(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 128}));
    CHECK(!lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 256}));

    void *first = lor_arena_alloc(&arena, 32);
    CHECK(first != NULL);
    CHECK(!lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 256}));
    CHECK(lor_arena_used(&arena) >= 32);

    size_t capacity = lor_arena_capacity(&arena);
    size_t committed = lor_arena_committed(&arena);
    CHECK(committed == capacity);

    lor_arena_reset(&arena);
    CHECK(lor_arena_used(&arena) == 0);
    CHECK(lor_arena_capacity(&arena) == capacity);
    CHECK(lor_arena_committed(&arena) == committed);

    void *second = lor_arena_alloc(&arena, 32);
    CHECK(second != NULL);
    CHECK(lor_arena_capacity(&arena) == capacity);
    CHECK(lor_arena_committed(&arena) == committed);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_mark_rewind(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 128}));
    CHECK(lor_arena_alloc(&arena, 24) != NULL);

    LorArenaMark mark = lor_arena_mark(&arena);
    size_t before = lor_arena_used(&arena);
    CHECK(lor_arena_alloc(&arena, 48) != NULL);
    CHECK(lor_arena_used(&arena) > before);

    lor_arena_rewind(&arena, mark);
    CHECK(lor_arena_used(&arena) == before);

    mark = lor_arena_mark(&arena);
    CHECK(lor_arena_alloc(&arena, 64) != NULL);
    CHECK(lor_arena_used(&arena) > before);
    lor_arena_rewind(&arena, mark);
    CHECK(lor_arena_used(&arena) == before);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_alignment(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 64}));

    void *ptr = lor_arena_alloc(&arena, 1);
    CHECK(ptr != NULL);
    CHECK(((uintptr_t)ptr % TEST_MAX_ALIGNMENT) == 0);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_zeroed_array_and_overflow(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 128}));

    unsigned char *bytes = (unsigned char *)lor_arena_alloc_zero(&arena, 16);
    CHECK(bytes != NULL);
    for (size_t i = 0; i < 16; ++i) CHECK(bytes[i] == 0);

    int *values = (int *)lor_arena_alloc_array_zero(&arena, 8, sizeof(*values));
    CHECK(values != NULL);

    for (size_t i = 0; i < 8; ++i) {
        CHECK(values[i] == 0);
        values[i] = (int)i;
    }

    CHECK(lor_arena_alloc(&arena, 0) == NULL);
    CHECK(lor_arena_alloc_array(&arena, 0, sizeof(int)) == NULL);
    CHECK(lor_arena_alloc_array(&arena, (size_t)-1, 2) == NULL);
    CHECK(lor_arena_alloc_array_zero(&arena, (size_t)-1, 2) == NULL);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_strdup_and_large_allocation(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 16}));

    char *copy = lor_arena_strdup(&arena, "liblor");
    CHECK(copy != NULL);
    CHECK(strcmp(copy, "liblor") == 0);

    void *large = lor_arena_alloc(&arena, 1024);
    CHECK(large != NULL);
    CHECK(lor_arena_capacity(&arena) >= 1024);
    CHECK(lor_arena_strdup(&arena, NULL) == NULL);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_virtual_backend(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){
                                            .backend = LOR_ARENA_BACKEND_VIRTUAL,
                                            .reserve_size = lor_page_size() * 4u,
                                            .commit_size = lor_page_size(),
                                        }));

    char *bytes = (char *)lor_arena_alloc(&arena, lor_page_size() + 32u);
    CHECK(bytes != NULL);
    bytes[0] = 'a';
    bytes[lor_page_size()] = 'b';
    CHECK(lor_arena_capacity(&arena) > lor_page_size());
    CHECK(lor_arena_committed(&arena) >= lor_page_size() + 32u);
    CHECK(lor_arena_committed(&arena) <= lor_arena_capacity(&arena));

    lor_arena_deinit(&arena);
    return 0;
}

static int test_scratch_arena(void) {
    LorScratch scratch = lor_scratch_begin(NULL);
    CHECK(scratch.arena != NULL);

    void *ptr = lor_arena_alloc(scratch.arena, 32);
    CHECK(ptr != NULL);

    LorScratch other = lor_scratch_begin(scratch.arena);
    CHECK(other.arena != NULL);
    CHECK(other.arena != scratch.arena);

    lor_scratch_end(other);
    lor_scratch_end(scratch);
    lor_scratch_cleanup();
    return 0;
}

int main(void) {
    CHECK(test_zero_initialized_arena() == 0);
    CHECK(test_reset_reuses_blocks() == 0);
    CHECK(test_mark_rewind() == 0);
    CHECK(test_alignment() == 0);
    CHECK(test_zeroed_array_and_overflow() == 0);
    CHECK(test_strdup_and_large_allocation() == 0);
    CHECK(test_virtual_backend() == 0);
    CHECK(test_scratch_arena() == 0);

    puts("test_arena: ok");
    return 0;
}
