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

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

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
    LorArena arena;
    void *first = NULL;
    void *second = NULL;
    size_t capacity = 0;

    lor_arena_init(&arena, 128);
    first = lor_arena_alloc(&arena, 32);
    CHECK(first != NULL);
    CHECK(lor_arena_used(&arena) >= 32);

    capacity = lor_arena_capacity(&arena);
    lor_arena_reset(&arena);
    CHECK(lor_arena_used(&arena) == 0);
    CHECK(lor_arena_capacity(&arena) == capacity);

    second = lor_arena_alloc(&arena, 32);
    CHECK(second != NULL);
    CHECK(lor_arena_capacity(&arena) == capacity);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_mark_rewind(void) {
    LorArena arena;
    size_t before = 0;

    lor_arena_init(&arena, 128);
    CHECK(lor_arena_alloc(&arena, 24) != NULL);
    CHECK(lor_arena_mark(&arena));
    before = lor_arena_used(&arena);
    CHECK(lor_arena_alloc(&arena, 48) != NULL);
    CHECK(lor_arena_used(&arena) > before);

    lor_arena_rewind(&arena);
    CHECK(lor_arena_used(&arena) == before);

    CHECK(lor_arena_mark(&arena));
    CHECK(lor_arena_alloc(&arena, 64) != NULL);
    CHECK(lor_arena_used(&arena) > before);
    lor_arena_rewind(&arena);
    CHECK(lor_arena_used(&arena) == before);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_alignment(void) {
    LorArena arena;
    void *ptr = NULL;

    lor_arena_init(&arena, 64);

    ptr = lor_arena_alloc(&arena, 1);
    CHECK(ptr != NULL);
    CHECK(((uintptr_t)ptr % TEST_MAX_ALIGNMENT) == 0);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_zeroed_array_and_overflow(void) {
    LorArena arena;
    int *values = NULL;
    size_t i = 0;

    lor_arena_init(&arena, 128);
    values = (int *)lor_arena_alloc_array_zero(&arena, 8, sizeof(*values));
    CHECK(values != NULL);

    for (i = 0; i < 8; ++i) {
        CHECK(values[i] == 0);
        values[i] = (int)i;
    }

    CHECK(lor_arena_alloc(&arena, 0) == NULL);
    CHECK(lor_arena_alloc_array(&arena, 0, sizeof(int)) == NULL);
    CHECK(lor_arena_alloc_array(&arena, (size_t)-1, 2) == NULL);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_strdup_and_large_allocation(void) {
    LorArena arena;
    char *copy = NULL;
    void *large = NULL;

    lor_arena_init(&arena, 16);
    copy = lor_arena_strdup(&arena, "liblor");
    CHECK(copy != NULL);
    CHECK(strcmp(copy, "liblor") == 0);

    large = lor_arena_alloc(&arena, 1024);
    CHECK(large != NULL);
    CHECK(lor_arena_capacity(&arena) >= 1024);
    CHECK(lor_arena_strdup(&arena, NULL) == NULL);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_virtual_backend(void) {
    LorArena arena = LOR_ARENA_INIT;
    LorArenaConfig config = {0};
    char *bytes = NULL;

    config.backend = LOR_ARENA_BACKEND_VIRTUAL;
    config.reserve_size = lor_page_size() * 4u;
    config.commit_size = lor_page_size();

    CHECK(lor_arena_init_ex(&arena, &config));
    bytes = (char *)lor_arena_alloc(&arena, lor_page_size() + 32u);
    CHECK(bytes != NULL);
    bytes[0] = 'a';
    bytes[lor_page_size()] = 'b';
    CHECK(lor_arena_capacity(&arena) > lor_page_size());
    CHECK(lor_arena_committed(&arena) >= lor_page_size() + 32u);

    lor_arena_deinit(&arena);
    return 0;
}

static int test_scratch_arena(void) {
    LorScratch scratch = lor_scratch_begin(NULL, 0);
    LorArena *conflicts[1];
    LorScratch other;
    void *ptr = NULL;

    CHECK(scratch.arena != NULL);
    ptr = lor_arena_alloc(scratch.arena, 32);
    CHECK(ptr != NULL);

    conflicts[0] = scratch.arena;
    other = lor_scratch_begin(conflicts, 1);
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
