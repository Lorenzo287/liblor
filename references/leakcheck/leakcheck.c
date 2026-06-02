#include <stdio.h>

// Define STB_LEAKCHECK_IMPLEMENTATION  before including the header.
#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"

typedef struct Node {
    int data;
    struct Node *next;
} Node;

Node *create_node(int data) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) { exit(1); }
    new_node->data = data;
    new_node->next = NULL;
    printf("Allocated a node for data %d at address %p\n", data,
           (void *)new_node);
    return new_node;
}

int main(void) {
    printf("--- Building a linked list ---\n");
    Node *head = create_node(10);
    head->next = create_node(20);
    head->next->next = create_node(30);
    printf("\nList created successfully.\n");

    printf(
        "\n--- Intentionally not freeing the list to demonstrate a leak ---\n");
    // free(head->next->next);
    // free(head->next);
    // free(head);

    printf(
        "\n--- Program finished. Calling stb_leakcheck_dumpmem() to report "
        "leaks. ---\n\n");

    // This function will print a report of all memory that was allocated
    // with `malloc` but not freed with `free`. It will point to the exact
    // file and line number of the allocation.
    stb_leakcheck_dumpmem();

    return 0;
}
