#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

int main(void) {
    Arena arena = ARENA_INIT;
    int *arr = arena_alloc_array(&arena, 10, sizeof(int), .zero = true);
    if (arr == NULL) {
        arena_deinit(&arena);
        return 1;
    }

    arr[10] = 69;
    printf("%d\n", arr[10]);

    leakcheck_report(stdout);
    getc(stdin);

    return 0;
}
