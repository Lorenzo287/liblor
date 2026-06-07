#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#include "../lor.h"

static void test_auto_free(void) {
    printf("--- LOR_AUTO_FREE ---\n");
    // LOR_AUTO_FREE automatically calls free() when ptr goes out of scope.
    // LOR_LEAKCHECK redefines malloc, so this allocation is tracked.
    LOR_AUTO_FREE void *ptr = malloc(100);
    printf("Allocated 100 bytes via malloc.\n");
    // Memory is freed automatically at the end of this function.
}

static void test_auto_arena(void) {
    printf("--- LOR_AUTO_ARENA ---\n");
    // LOR_AUTO_ARENA automatically calls lor_arena_deinit() at scope exit.
    LOR_AUTO_ARENA LorArena arena = LOR_ARENA_INIT;
    lor_arena_alloc(&arena, 1024);
    printf("Allocated 1024 bytes in the arena.\n");
    // lor_arena_deinit(&arena) is called automatically at the end of this function.
}

static void test_auto_scratch(void) {
    printf("--- LOR_AUTO_SCRATCH ---\n");
    // LOR_AUTO_SCRATCH automatically calls lor_scratch_end() at scope exit.
    LOR_AUTO_SCRATCH LorScratch scratch = lor_scratch_begin(NULL, 0);
    if (scratch.arena != NULL) {
        lor_arena_alloc(scratch.arena, 512);
        printf("Allocated 512 bytes in the scratch arena.\n");
    }
    // lor_scratch_end(scratch) is called automatically at the end of this function.
}

static void test_auto_mmap(void) {
    printf("--- LOR_AUTO_MMAP ---\n");
    const char *dummy_path = "cleanup_dummy.txt";
    
    FILE *f = fopen(dummy_path, "wb");
    if (f) {
        fputs("auto mmap test", f);
        fclose(f);
    }

    {
        // LOR_AUTO_MMAP automatically calls lor_mmap_unmap() at scope exit.
        LOR_AUTO_MMAP LorMmap map = lor_mmap_file(dummy_path, LOR_MMAP_READ);
        if (map.data != NULL) {
            printf("Mapped %zu bytes successfully.\n", map.size);
        }
        // lor_mmap_unmap(&map) is called automatically here.
    }
    
    remove(dummy_path);
}

static void test_auto_file(void) {
    printf("--- LOR_AUTO_FILE ---\n");
    const char *dummy_path = "cleanup_file.txt";
    {
        // LOR_AUTO_FILE automatically calls fclose() at scope exit.
        LOR_AUTO_FILE FILE *f = fopen(dummy_path, "wb");
        if (f) {
            fputs("auto file test", f);
            printf("Opened temporary file successfully.\n");
        }
        // fclose(f) is called automatically here.
    }
    remove(dummy_path);
}

static void test_auto_string(void) {
    printf("--- LOR_AUTO_STRING ---\n");
    LOR_AUTO_STRING LorString text = LOR_STRING_INIT;
    if (lor_string_append_cstr(&text, "automatic string") == LOR_STATUS_OK)
        printf("Built: %s\n", text);
    // lor_string_deinit(&text) is called automatically at scope exit.
}

int main(void) {
    printf("Starting auto cleanup tests...\n\n");

    test_auto_free();
    test_auto_arena();
    test_auto_scratch();
    test_auto_mmap();
    test_auto_file();
    test_auto_string();

    printf("\nAll scopes exited. The leak checker should report 0 leaks below:\n");
    size_t leaks = lor_leakcheck_report(stdout);
    printf("Total leaks: %zu\n", leaks);

    return 0;
}
