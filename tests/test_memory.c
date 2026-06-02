// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "lor/memory.h"

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

static int test_heap_helpers_and_leakcheck(void) {
    char *text = NULL;
    int *values = NULL;
    LorLeakStats stats;

    lor_leakcheck_enable(1);
    CHECK(lor_leakcheck_count() == 0);

    text = lor_strdup_debug("liblor", __FILE__, __LINE__);
    CHECK(text != NULL);
    CHECK(strcmp(text, "liblor") == 0);
    CHECK(lor_leakcheck_count() == 1);

    text = (char *)lor_realloc_debug(text, 32, __FILE__, __LINE__);
    CHECK(text != NULL);
    CHECK(strcmp(text, "liblor") == 0);
    stats = lor_leakcheck_stats();
    CHECK(stats.heap_count == 1);
    CHECK(stats.heap_bytes == 32);

    values = (int *)lor_calloc_debug(4, sizeof(*values), __FILE__, __LINE__);
    CHECK(values != NULL);
    CHECK(values[0] == 0);
    CHECK(lor_leakcheck_count() == 2);

    lor_free(values);
    lor_free(text);
    CHECK(lor_leakcheck_count() == 0);
    lor_leakcheck_enable(0);
    return 0;
}

static int test_leakcheck_tracks_arena_virtual_and_mmap_lifetimes(void) {
    LorArena arena = LOR_ARENA_INIT;
    LorVirtualMemory memory = {0};
    LorLeakStats stats;
    FILE *file = NULL;
    LorMmap map = {0};
    LorMmap copy = {0};

    lor_leakcheck_enable(1);

    lor_arena_init(&arena, 128);
    CHECK(lor_arena_alloc(&arena, 16) != NULL);
    stats = lor_leakcheck_stats();
    CHECK(stats.arena_count == 1);

    memory = lor_virtual_alloc(lor_page_size() * 2u, lor_page_size());
    CHECK(memory.ptr != NULL);
    stats = lor_leakcheck_stats();
    CHECK(stats.virtual_count == 1);

    file = fopen(".build/lor_mmap_leakcheck.txt", "wb");
    CHECK(file != NULL);
    CHECK(fputs("mapped", file) >= 0);
    CHECK(fclose(file) == 0);

    map = lor_mmap_file(".build/lor_mmap_leakcheck.txt", LOR_MMAP_READ);
    CHECK(map.data != NULL);
    stats = lor_leakcheck_stats();
    CHECK(stats.mmap_count == 1);

    copy = lor_mmap_file(".build/lor_mmap_leakcheck.txt", LOR_MMAP_COPY);
    CHECK(copy.data != NULL);
    ((char *)copy.data)[0] = 'M';
    stats = lor_leakcheck_stats();
    CHECK(stats.mmap_count == 2);

    lor_mmap_unmap(&copy);
    lor_mmap_unmap(&map);
    lor_virtual_release(&memory);
    lor_arena_deinit(&arena);
    CHECK(lor_leakcheck_count() == 0);
    lor_leakcheck_enable(0);
    return 0;
}

int main(void) {
    CHECK(test_heap_helpers_and_leakcheck() == 0);
    CHECK(test_leakcheck_tracks_arena_virtual_and_mmap_lifetimes() == 0);

    puts("test_memory: ok");
    return 0;
}
