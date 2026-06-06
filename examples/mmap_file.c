#define _CRT_SECURE_NO_WARNINGS
#include "lor/memory.h"
#include <stdio.h>

int main(void) {
    const char *path = "lor_mmap_example.tmp";

    FILE *file = fopen(path, "wb");
    if (file == NULL) return 1;

    if (fputs("hello from lor_mmap_file\n", file) < 0) {
        fclose(file);
        remove(path);
        return 1;
    }
    if (fclose(file) != 0) {
        remove(path);
        return 1;
    }

    LorMmap map = lor_mmap_file(path, LOR_MMAP_READ);
    if (map.data == NULL) {
        remove(path);
        return 1;
    }

    printf("mapped %zu bytes: %.*s", map.size, (int)map.size, (char *)map.data);

    lor_mmap_unmap(&map);
    remove(path);
    return 0;
}
