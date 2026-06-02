#include <fcntl.h>
#include <stdlib.h>
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"

#define BUFFER_SIZE INT_MAX

int copy_goto(const char *src_path, const char *dst_path) {
    int src, dst;
    char *buffer;
    int ret = 0, n;

    buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        ret = 1;
        goto out;
    }

    src = open(src_path, O_RDONLY);
    if (src < 0) {
        ret = 2;
        goto out_free;
    }

    dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) {
        ret = 3;
        goto out_src;
    }

    while (1) {
        n = read(src, buffer, BUFFER_SIZE);
        if (n == 0) break;
        if (n < 0) {
            ret = 4;
            goto out_dst;
        }

        if (write(dst, buffer, n) != n) {
            ret = 5;
            goto out_dst;
        }
    }

out_dst:
    close(dst);
out_src:
    close(src);
out_free:
    free(buffer);
out:
    return ret;
}

int main(void) {
    int ret = copy_goto("copy_goto.c", "copy_of_copy.txt");
    stb_leakcheck_dumpmem();
    return ret;
}
