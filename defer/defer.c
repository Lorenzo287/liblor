#include "defer.h"
#include <stdio.h>

int main(void) {
    defer {
        putchar('!');
        putchar('\n');
    }
    defer {
        printf("World");
    }

    {
        defer {
            printf("defer inside scope\n");
        }
        printf("inside scope\n");
    }

    printf("Hello, ");
    return 0;
}
