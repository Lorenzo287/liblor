#include <fcntl.h>
#include <stdlib.h>
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"

#define BUFFER_SIZE INT_MAX

int copy_classic(const char *src_path, const char *dst_path) {
    int src, dst;
    char *buffer;
    int n;

    buffer = malloc(BUFFER_SIZE);
    if (!buffer) return 1;

    src = open(src_path, O_RDONLY);
    if (src < 0) {
        free(buffer);
        return 2;
    }

    dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) {
        close(src);
        free(buffer);
        return 3;
    }

    while (1) {
        n = read(src, buffer, BUFFER_SIZE);
        if (n == 0) break;
        if (n < 0) {
            close(dst);
            close(src);
            free(buffer);
            return 4;
        }

        if (write(dst, buffer, n) != n) {
            close(dst);
            close(src);
            free(buffer);
            return 5;
        }
    }

    close(dst);
    close(src);
    free(buffer);
    return 0;
}

int main(void) {
    int ret = copy_classic("copy_classic.c", "copy_of_copy.txt");
    stb_leakcheck_dumpmem();
    return ret;
}
