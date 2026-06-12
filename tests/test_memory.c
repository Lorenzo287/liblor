// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "lor/memory.h"

#include <stdio.h>

#include "lor_test.h"

static int test_leakcheck_tracks_arena_and_mmap_lifetimes(void) {
#if defined(LOR_LEAKCHECK)
    CHECK(lor_leakcheck_is_enabled());
#else
    CHECK(!lor_leakcheck_is_enabled());
#endif

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

static int test_mmap_modes(void) {
    const char *path = ".build/lor_mmap_modes.txt";
    FILE *file = fopen(path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("mapped", file) >= 0);
    CHECK(fclose(file) == 0);

    LorMmap copy = lor_mmap_file(path, LOR_MMAP_COPY);
    CHECK(copy.data != NULL);
    CHECK(copy.size == 6u);
    ((char *)copy.data)[0] = 'C';
    lor_mmap_unmap(&copy);

    file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fgetc(file) == 'm');
    CHECK(fclose(file) == 0);

    LorMmap shared = lor_mmap_file(path, LOR_MMAP_SHARED);
    CHECK(shared.data != NULL);
    CHECK(shared.size == 6u);
    ((char *)shared.data)[0] = 'S';
    lor_mmap_unmap(&shared);

    file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fgetc(file) == 'S');
    CHECK(fclose(file) == 0);
    CHECK(remove(path) == 0);
    return 0;
}

static int test_mmap_failures_and_repeated_unmap(void) {
    const char *empty_path = ".build/lor_mmap_empty.txt";
    const char *missing_path = ".build/lor_mmap_missing.txt";
    FILE *file;

    (void)remove(missing_path);
    CHECK(lor_mmap_file(NULL, LOR_MMAP_READ).data == NULL);
    CHECK(lor_mmap_file(missing_path, LOR_MMAP_READ).data == NULL);

    file = fopen(empty_path, "wb");
    CHECK(file != NULL);
    CHECK(fclose(file) == 0);
    CHECK(lor_mmap_file(empty_path, LOR_MMAP_READ).data == NULL);
    CHECK(remove(empty_path) == 0);

    file = fopen(missing_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("mapped", file) >= 0);
    CHECK(fclose(file) == 0);
    CHECK(lor_mmap_file(missing_path, (LorMmapMode)99).data == NULL);

    LorMmap map = lor_mmap_file(missing_path, LOR_MMAP_READ);
    CHECK(map.data != NULL);
    lor_mmap_unmap(&map);
    CHECK(map.data == NULL);
    CHECK(map.size == 0);
    lor_mmap_unmap(&map);
    lor_mmap_unmap(NULL);
    CHECK(remove(missing_path) == 0);
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
    CHECK(test_mmap_modes() == 0);
    CHECK(test_mmap_failures_and_repeated_unmap() == 0);
    CHECK(test_cleanup_file_closes_and_clears_pointer() == 0);

    puts("test_memory: ok");
    return 0;
}
