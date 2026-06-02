#include <errno.h>

// NOTE: before cleanup.h so that free() is correctly redefined
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"

#include "cleanup.h"

#define BUFFER_SIZE INT_MAX

int copy_cleanup(const char *src_path, const char *dst_path) {
    // COOL: try to remove the attributes
    _cleanup_close_ int src = -EBADF, dst = -EBADF;
    _cleanup_free_ char *buffer = NULL;
    int n;

    buffer = malloc(BUFFER_SIZE);
    if (!buffer) return 1;

    src = open(src_path, O_RDONLY);
    if (src < 0) return 2;

    dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) return 3;

    while (1) {
        n = read(src, buffer, BUFFER_SIZE);
        if (n == 0) return 0;
        if (n < 0) return 4;

        if (write(dst, buffer, n) != n) return 5;
    }
}

int main(void) {
    int ret = copy_cleanup("copy_cleanup.c", "copy_of_copy.txt");
    stb_leakcheck_dumpmem();
    return ret;
}
