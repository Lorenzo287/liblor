#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

int main(void) {
    printf("\x1b[96mSCENARIO 1: STDLIB HEAP (MALLOC/CALLOC)\x1b[0m\n");

    // By defining LOR_LEAKCHECK, liblor overrides standard malloc, calloc, 
    // realloc, and free with macros that inject file and line number.
    void *ptr1 = malloc(128);
    void *ptr2 = calloc(10, sizeof(int));
    
    // We can also realloc; the leak checker will correctly track the updated 
    // size and the new location of the allocation.
    ptr1 = realloc(ptr1, 256);

    printf("Allocated 256 bytes via realloc, and 40 bytes via calloc.\n");
    printf("The leak checker reports:\n");
    leakcheck_report(stdout);

    // Freeing them dynamically removes them from the leak check tracker.
    free(ptr1);
    free(ptr2);

    printf("\nAfter freeing both pointers, leakcheck count is: %zu\n", leakcheck_count());


    printf("\n\x1b[96mSCENARIO 2: MEMORY MAPPED FILES (MMAP)\x1b[0m\n");

    // Let's create a temporary dummy file to map.
    const char *dummy_path = "leakcheck_dummy.txt";
    FILE *f = fopen(dummy_path, "wb");
    if (f) {
        fputs("Hello, mapped world!", f);
        fclose(f);
    }

    // Mapping the file using liblor's mmap wrapper.
    // The leak checker intercepts this call to track the mapping lifetime.
    Mmap map = mmap_file(dummy_path, MMAP_READ);
    if (map.data != NULL) {
        printf("Successfully mapped file of size %zu bytes.\n", map.size);
        printf("The leak checker explicitly tracks mmap resources too:\n");
        leakcheck_report(stdout);

        // Unmapping removes it from the tracker.
        mmap_unmap(&map);
    } else {
        printf("Failed to map file.\n");
    }

    // Clean up the temporary file on disk.
    remove(dummy_path);

    return 0;
}
