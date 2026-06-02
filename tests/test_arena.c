// SPDX-License-Identifier: MIT

#include "lor/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static int test_alignment(void) {
    LorArena arena;
    size_t alignment = 0;

    lor_arena_init(&arena, 64);

    for (alignment = 1; alignment <= 64; alignment *= 2) {
        void *ptr = lor_arena_alloc_aligned(&arena, 1, alignment);
        CHECK(ptr != NULL);
        CHECK(((uintptr_t)ptr % alignment) == 0);
    }

    CHECK(lor_arena_alloc_aligned(&arena, 8, 3) == NULL);

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

int main(void) {
    CHECK(test_zero_initialized_arena() == 0);
    CHECK(test_reset_reuses_blocks() == 0);
    CHECK(test_alignment() == 0);
    CHECK(test_zeroed_array_and_overflow() == 0);
    CHECK(test_strdup_and_large_allocation() == 0);

    puts("test_arena: ok");
    return 0;
}
