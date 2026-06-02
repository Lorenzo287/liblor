#include <stdio.h>
#include <stdlib.h>
#include "custom_main.h"

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

// NOTE: cool trick to redefine main
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
    return 0;
}
