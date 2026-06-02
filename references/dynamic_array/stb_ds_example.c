#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

int main(void) {
    int *arr = NULL;

    arrpush(arr, 5);
    arrpush(arr, 4);
    arrpush(arr, 7);
    arrpush(arr, 9);
    arrpush(arr, 2);

    printf("arr length: %zu\n", arrlen(arr));
    printf("arr capacity: %zu\n\n", arrcap(arr));

    size_t len = arrlen(arr);
    for (size_t i = 0; i < len; i++)
        printf("arr[%zu]: %d\n", len - i - 1, arrpop(arr));

    arrfree(arr);
    return 0;
}
