#include <stdio.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

typedef struct {
    const char *key;
    size_t value;
} Item;

int main() {
    Item *table = NULL;

    shput(table, "foo", 69);
    shput(table, "bar", 420);
    shput(table, "baz", 1337);
    ptrdiff_t idx = shgeti(table, "bar");
    if (idx >= 0) { table[idx].value = 80085; }

    for (int i = 0; i < shlen(table); ++i) {
        printf("%s => %zu\n", table[i].key, table[i].value);
    }
    return 0;
}
