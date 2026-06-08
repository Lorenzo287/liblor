// SPDX-License-Identifier: MIT

#include "lor/type.h"

#include <stdio.h>

typedef struct Point {
    int x;
    int y;
} Point;

int main(void) {
    Point point = {3, 4};

    printf("42: %s\n", lor_type_name(42));
    printf("3.14: %s\n", lor_type_name(3.14));
    printf("\"liblor\": %s\n", lor_type_name("liblor"));
    printf("Point: %s\n", lor_type_name(point));

    LorStringView view = LOR_SV_LITERAL("view");
    LorArena arena = LOR_ARENA_INIT;
    LorRandom random = LOR_RANDOM_INIT;
    printf("view: %s\n", lor_type_name(view));
    printf("arena: %s\n", lor_type_name(arena));
    printf("random: %s\n", lor_type_name(random));

#if LOR_HAS_TYPEOF
    lor_typeof(point) copy = point;
    printf("copy: Point(%d, %d)\n", copy.x, copy.y);
#endif

    return 0;
}
