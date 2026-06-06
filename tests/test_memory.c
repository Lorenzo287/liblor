// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "lor/memory.h"

#include <stdio.h>

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #expr);                                                  \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_leakcheck_tracks_arena_and_mmap_lifetimes(void) {
    LorArena arena = LOR_ARENA_INIT;
    CHECK(lor_arena_init_config(&arena, (LorArenaConfig){.block_size = 128}));
    CHECK(lor_arena_alloc(&arena, 16) != NULL);

    LorLeakStats stats = lor_leakcheck_stats();
#if defined(LOR_LEAKCHECK)
    CHECK(stats.arena_count == 1);
#else
    CHECK(stats.arena_count == 0);
#endif

    FILE *file = fopen(".build/lor_mmap_leakcheck.txt", "wb");
    CHECK(file != NULL);
    CHECK(fputs("mapped", file) >= 0);
    CHECK(fclose(file) == 0);

    LorMmap map = lor_mmap_file(".build/lor_mmap_leakcheck.txt", LOR_MMAP_READ);
    CHECK(map.data != NULL);
    stats = lor_leakcheck_stats();
#if defined(LOR_LEAKCHECK)
    CHECK(stats.mmap_count == 1);
#else
    CHECK(stats.mmap_count == 0);
#endif

    LorMmap copy = lor_mmap_file(".build/lor_mmap_leakcheck.txt", LOR_MMAP_COPY);
    CHECK(copy.data != NULL);
    ((char *)copy.data)[0] = 'M';
    stats = lor_leakcheck_stats();
#if defined(LOR_LEAKCHECK)
    CHECK(stats.mmap_count == 2);
#else
    CHECK(stats.mmap_count == 0);
#endif

    lor_mmap_unmap(&copy);
    lor_mmap_unmap(&map);
    lor_arena_deinit(&arena);
    CHECK(lor_leakcheck_count() == 0);
    return 0;
}

static int test_cleanup_file_closes_and_clears_pointer(void) {
#if LOR_CLEANUP_SUPPORTED
    {
        LOR_AUTO_FILE FILE *file = fopen(".build/lor_cleanup_file.txt", "wb");
        CHECK(file != NULL);
        CHECK(fputs("cleanup", file) >= 0);
    }
#endif

    return 0;
}

int main(void) {
    CHECK(test_leakcheck_tracks_arena_and_mmap_lifetimes() == 0);
    CHECK(test_cleanup_file_closes_and_clears_pointer() == 0);

    puts("test_memory: ok");
    return 0;
}
