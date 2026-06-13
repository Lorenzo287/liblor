#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

int main(void) {
    int *queue = ARRAY_INIT;

    printf("--- Queue Example ---\n");
    for (int i = 1; i <= 3; ++i) {
        int val = i * 10;
        array_push(queue, val);
        printf("Enqueued: %d\n", val);
    }

    int out_val;
    // NOTE: removing at index 0 requires shifting all remaining elements
    // to the left, which is an O(N) operation. For high-performance queues
    // that process a massive amount of elements, consider using a custom
    // ring buffer wrapper. This dynamic array approach works fine for
    // small queues where the O(N) penalty is negligible.
    while (array_size(queue) > 0) {
        array_remove(queue, 0, &out_val);
        printf("Dequeued: %d\n", out_val);
    }
    
    array_deinit(&queue);

    int *stack = ARRAY_INIT;

    printf("\n--- Stack Example ---\n");
    for (int i = 1; i <= 3; ++i) {
        int val = i * 100;
        array_push(stack, val);
        printf("Pushed: %d\n", val);
    }

    // NOTE: array_pop removes elements from the end of the array.
    // This is an O(1) operation, making it highly efficient
    // for implementing a stack.
    while (array_pop(stack, &out_val)) {
        printf("Popped: %d\n", out_val);
    }

    array_deinit(&stack);
    
    printf("\nMemory leaks: %d\n", (int)leakcheck_report(stdout));
    return 0;
}
