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
    LOR_AUTO_SCRATCH LorScratch scratch = lor_scratch_begin(NULL);
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

static void test_auto_array(void) {
    printf("--- LOR_AUTO_ARRAY ---\n");
    LOR_AUTO_ARRAY int *numbers = LOR_ARRAY_INIT;
    if (lor_array_push_as(numbers, int, 10) == LOR_STATUS_OK &&
        lor_array_push_as(numbers, int, 20) == LOR_STATUS_OK)
        printf("Built array with %zu elements.\n", lor_array_size(numbers));
    // lor_array_deinit(&numbers) is called automatically at scope exit.
}

typedef LOR_MAP_ENTRY(int, int) CleanupMapEntry;

static void test_auto_map(void) {
    printf("--- LOR_AUTO_MAP ---\n");
    LOR_AUTO_MAP CleanupMapEntry *map = LOR_MAP_INIT;
    if (lor_map_put_as(map, CleanupMapEntry, 10, 20) == LOR_STATUS_OK)
        printf("Built map with %zu entry.\n", lor_map_size(map));
    // lor_map_deinit(&map) is called automatically at scope exit.
}

static void test_auto_set(void) {
    printf("--- LOR_AUTO_SET ---\n");
    LOR_AUTO_SET int *set = LOR_SET_INIT;
    if (lor_set_add_as(set, int, 10) == LOR_STATUS_OK)
        printf("Built set with %zu key.\n", lor_set_size(set));
    // lor_set_deinit(&set) is called automatically at scope exit.
}

int main(void) {
    printf("Starting auto cleanup tests...\n\n");

    test_auto_free();
    test_auto_arena();
    test_auto_scratch();
    test_auto_mmap();
    test_auto_file();
    test_auto_string();
    test_auto_array();
    test_auto_map();
    test_auto_set();

    printf("\nAll scopes exited. The leak checker should report 0 leaks below:\n");
    size_t leaks = lor_leakcheck_report(stdout);
    printf("Total leaks: %zu\n", leaks);

    return 0;
}
