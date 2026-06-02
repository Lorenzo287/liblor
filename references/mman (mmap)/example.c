#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "mman.h"

// cd mman-win32-final\ && gcc example.c mman.c -o example.exe && example

int main() {
    const char *filename = "example.txt";
    int fd;
    size_t size = 100;
    void *map;

    // 1. Create a dummy file for mapping
    fd = _open(filename, _O_RDWR | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    // Expand file size to 100 bytes
    _chsize(fd, size);

    // 2. Map the file into memory
    map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap failed");
        _close(fd);
        return 1;
    }

    // 3. Write to the mapped memory
    sprintf((char *)map, "Hello, Memory Mapped File!");
    printf("Successfully wrote to memory: %s\n", (char *)map);

    // 4. Cleanup
    if (munmap(map, size) == -1) {
        perror("munmap failed");
    }
    _close(fd);

    printf("Cleanup complete.\n");
    return 0;
}
