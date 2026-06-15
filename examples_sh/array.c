#include <stdio.h>

#define LOR_IMPLEMENTATION
#include "../lor.h"

int main(void) {
    assert(LOR_ARRAY_INIT == NULL);
    int *array = LOR_ARRAY_INIT;

    int val = 1;
    lor_array_push(array, val);
    lor_array_push(array, (int){val + 1});
    lor_array_push(array, (int){3});

    printf("=== PUSH SOME VALUES ===\nsize: %d, capacity: %d\n",
           (int)lor_array_size(array), (int)lor_array_capacity(array));
    for (size_t i = 0; i < lor_array_size(array); i++) printf("%d ", array[i]);

    printf("\n\n=== REMOVE SOME ===\n");
    int popped;
    lor_array_pop(array, &popped);
    printf("popped: %d\n", popped);
	
    int head;
    lor_array_remove(array, 0, &head);
    printf("head: %d\n", head);

    lor_array_shrink_to_fit(array);
    printf("size: %d, capacity after shrink: %d\n", (int)lor_array_size(array),
           (int)lor_array_capacity(array));

	printf("\n=== APPEND SOME ===\n");
    int elements[] = {69, 69, 69};
    lor_array_append(array, elements, sizeof(elements) / sizeof(*elements));
    lor_print(lor_print_array(array));
    printf("new size: %d, new capacity: %d\n", (int)lor_array_size(array),
           (int)lor_array_capacity(array));

	printf("\n=== INSERT MANY ===\n");
	int list[5] = {0};
	lor_array_insert_many(array, 1, list, 5);
    lor_print(lor_print_array(array));

	lor_array_deinit(&array);
    return 0;
}
