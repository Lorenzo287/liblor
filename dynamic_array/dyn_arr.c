#include <stdio.h>
#include <stdlib.h>

// NOTE: this is not a stb library, this is code from tsoding,
// he was inspired by stb since they use prefix headers in stb_ds.h
// (see antirez/prefixed_len_string)

typedef struct {
    size_t count;
    size_t capacity;
} Header;

#define ARR_INIT_CAP 1

#define arr_push(arr, x)                                                   \
    do {                                                                   \
        if (arr == NULL) {                                                 \
            Header *header =                                               \
                malloc(sizeof(*arr) * ARR_INIT_CAP + sizeof(Header));      \
            header->count = 0;                                             \
            header->capacity = ARR_INIT_CAP;                               \
            arr = (void *)(header + 1);                                    \
        }                                                                  \
        Header *header = (Header *)(arr) - 1;                              \
        if (header->count >= header->capacity) {                           \
            header->capacity *= 2;                                         \
            header = realloc(                                              \
                header, sizeof(*arr) * header->capacity + sizeof(Header)); \
            arr = (void *)(header + 1);                                    \
        }                                                                  \
        (arr)[header->count++] = (x);                                      \
    } while (0)

#define arr_len(arr) ((Header *)(arr) - 1)->count

#define arr_free(arr) free((Header *)(arr) - 1)

int main() {
    // NOTE: because we use MACROS they are type insensitive!
    // eg could do int *numbers = NULL
    float *numbers = NULL;
    arr_push(numbers, 69);
    arr_push(numbers, 420);
    arr_push(numbers, 1337);
    arr_push(numbers, 80085);
    if (1)
        arr_push(numbers, 111);
    else
        arr_push(numbers, 222);
    for (size_t i = 0; i < arr_len(numbers); ++i) {
        printf("%f\n", numbers[i]);
    }
    arr_free(numbers);
    return 0;
}
