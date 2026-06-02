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

static int test_heap_allocator_allocates_and_frees(void) {
    LorAllocator allocator = lor_allocator_heap();
    unsigned char *bytes = NULL;
    size_t i = 0;

    CHECK(lor_allocator_is_valid(allocator));

    bytes = (unsigned char *)lor_allocator_alloc(allocator, 32);
    CHECK(bytes != NULL);

    for (i = 0; i < 32; ++i) bytes[i] = (unsigned char)i;
    for (i = 0; i < 32; ++i) CHECK(bytes[i] == (unsigned char)i);

    lor_allocator_free(allocator, bytes, 32);
    return 0;
}

static int test_zeroed_allocation(void) {
    LorAllocator allocator = lor_allocator_heap();
    int *values =
        (int *)lor_allocator_alloc_array_zero(allocator, 8, sizeof(*values));
    size_t i = 0;

    CHECK(values != NULL);
    for (i = 0; i < 8; ++i) CHECK(values[i] == 0);

    lor_allocator_free(allocator, values, 8 * sizeof(*values));
    return 0;
}

static int test_realloc_grows_and_preserves_data(void) {
    LorAllocator allocator = lor_allocator_heap();
    char *text = (char *)lor_allocator_alloc(allocator, 6);

    CHECK(text != NULL);
    memcpy(text, "hello", 6);

    text = (char *)lor_allocator_realloc(allocator, text, 6, 32);
    CHECK(text != NULL);
    CHECK(strcmp(text, "hello") == 0);

    lor_allocator_free(allocator, text, 32);
    return 0;
}

static int test_realloc_array_shrinks_and_frees(void) {
    LorAllocator allocator = lor_allocator_heap();
    int *values = (int *)lor_allocator_alloc_array(allocator, 16, sizeof(*values));
    size_t i = 0;

    CHECK(values != NULL);
    for (i = 0; i < 16; ++i) values[i] = (int)i;

    values = (int *)lor_allocator_realloc_array(allocator, values, 16, 4,
                                                sizeof(*values));
    CHECK(values != NULL);
    for (i = 0; i < 4; ++i) CHECK(values[i] == (int)i);

    values =
        (int *)lor_allocator_realloc_array(allocator, values, 4, 0, sizeof(*values));
    CHECK(values == NULL);
    return 0;
}

static int test_invalid_and_overflow_behavior(void) {
    LorAllocator invalid = LOR_ALLOCATOR_INIT;
    LorAllocator allocator = lor_allocator_heap();

    CHECK(!lor_allocator_is_valid(invalid));
    CHECK(lor_allocator_alloc(invalid, 16) == NULL);
    CHECK(lor_allocator_alloc_zero(invalid, 16) == NULL);
    CHECK(lor_allocator_realloc(invalid, NULL, 0, 16) == NULL);
    lor_allocator_free(invalid, NULL, 0);

    CHECK(lor_allocator_alloc(allocator, 0) == NULL);
    CHECK(lor_allocator_alloc_array(allocator, 0, sizeof(int)) == NULL);
    CHECK(lor_allocator_alloc_array(allocator, 4, 0) == NULL);
    CHECK(lor_allocator_alloc_array(allocator, (size_t)-1, 2) == NULL);
    CHECK(lor_allocator_alloc_array_zero(allocator, (size_t)-1, 2) == NULL);
    CHECK(lor_allocator_realloc_array(allocator, NULL, (size_t)-1, 1, 2) == NULL);
    return 0;
}

int main(void) {
    CHECK(test_heap_allocator_allocates_and_frees() == 0);
    CHECK(test_zeroed_allocation() == 0);
    CHECK(test_realloc_grows_and_preserves_data() == 0);
    CHECK(test_realloc_array_shrinks_and_frees() == 0);
    CHECK(test_invalid_and_overflow_behavior() == 0);

    puts("test_allocator: ok");
    return 0;
}
