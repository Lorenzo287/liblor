// SPDX-License-Identifier: MIT

#include "lor/set.h"

#include <stdio.h>

static void print_set(const char *name, const int *set) {
    printf("%s = {", name);
    for (size_t i = 0; i < lor_set_size(set); ++i)
        printf("%s%d", i == 0 ? "" : ", ", set[i]);
    puts("}");
}

int main(void) {
    LOR_AUTO_SET int *a = LOR_SET_INIT;
    LOR_AUTO_SET int *b = LOR_SET_INIT;
    for (int i = 1; i <= 4; ++i)
        if (lor_set_add(a, i) != LOR_STATUS_OK) return 1;
    for (int i = 3; i <= 6; ++i)
        if (lor_set_add(b, i) != LOR_STATUS_OK) return 1;

    LOR_AUTO_SET int *combined = LOR_SET_INIT;
    LOR_AUTO_SET int *common = LOR_SET_INIT;
    LOR_AUTO_SET int *only_a = LOR_SET_INIT;
    LOR_AUTO_SET int *exclusive = LOR_SET_INIT;

    if (lor_set_union(combined, a, b) != LOR_STATUS_OK ||
        lor_set_intersection(common, a, b) != LOR_STATUS_OK ||
        lor_set_difference(only_a, a, b) != LOR_STATUS_OK ||
        lor_set_symmetric_difference(exclusive, a, b) != LOR_STATUS_OK)
        return 1;

    print_set("a | b", combined);
    print_set("a & b", common);
    print_set("a - b", only_a);
    print_set("a ^ b", exclusive);
    return 0;
}
