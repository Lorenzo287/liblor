// SPDX-License-Identifier: MIT

#include "lor/array.h"

#include <stdio.h>

typedef struct Point {
    int x;
    int y;
} Point;

int main(void) {
    LOR_AUTO_ARRAY Point *points = LOR_ARRAY_INIT;

    for (int i = 0; i < 5; ++i) {
        Point point = {.x = i, .y = i * i};
        if (lor_array_push(points, point) != LOR_STATUS_OK) return 1;
    }

#if LOR_HAS_ARRAY_PUSH_AUTO
    if (lor_array_push_auto(points, ((Point){.x = 10, .y = 100})) !=
        LOR_STATUS_OK)
        return 1;
#else
    if (lor_array_push_as(points, Point, .x = 10, .y = 100) !=
        LOR_STATUS_OK)
        return 1;
#endif

    if (lor_array_insert_as(points, 2, Point, .x = 20, .y = 400) !=
        LOR_STATUS_OK)
        return 1;

    for (size_t i = 0; i < lor_array_size(points); ++i)
        printf("points[%zu] = {%d, %d}\n", i, points[i].x, points[i].y);

    Point last;
    if (lor_array_pop(points, &last))
        printf("popped = {%d, %d}\n", last.x, last.y);

    printf("size=%zu capacity=%zu\n", lor_array_size(points),
           lor_array_capacity(points));
    return 0;
}
