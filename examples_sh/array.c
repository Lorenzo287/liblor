#include <stdio.h>

#define LOR_IMPLEMENTATION
#include "../lor.h"

int main(void) {
    assert(LOR_ARRAY_INIT == NULL);
    int *array = LOR_ARRAY_INIT;

    int val = 7;
    lor_array_push(array, val);
    lor_array_push(array, (int){val + 1});
    lor_array_push(array, (int){9});

	for (size_t i = 0; i < lor_array_size(array); i++)
		printf("%d ", array[i]);
	printf("\n");
    return 0;
}
