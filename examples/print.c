// SPDX-License-Identifier: MIT

#include "lor/print.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct Point {
    int x;
    int y;
} Point;

typedef LOR_MAP_ENTRY(LorStringView, int) Score;

static LorStatus print_point(FILE *out, const void *value) {
    const Point *point = value;
    return fprintf(out, "Point(%d, %d)", point->x, point->y) < 0
               ? LOR_STATUS_SYSTEM_ERROR
               : LOR_STATUS_OK;
}

int main(void) {
    lor_print("answer", 42, (bool)true, 3.14);
    lor_print("same line", lor_end(" ... "));
    lor_print("continued");

    LorStringView view = LOR_SV_LITERAL("exact-length view");
    lor_print(view);

    LorString owned = LOR_STRING_INIT;
    lor_string_assign_cstr(&owned, "owned string");
    lor_print(owned, lor_string_view(owned));

    int *numbers = LOR_ARRAY_INIT;
    lor_array_push_as(numbers, int, 10);
    lor_array_push_as(numbers, int, 20);
    lor_print("array", lor_print_array(numbers));

    LorStringView *tags = LOR_SET_INIT;
    LorStringView fast = LOR_SV_LITERAL("fast");
    LorStringView readable = LOR_SV_LITERAL("readable");
    lor_set_add(tags, fast);
    lor_set_add(tags, readable);
    lor_print("set", lor_print_set(tags));

    Score *scores = LOR_MAP_INIT;
    LorStringView alice = LOR_SV_LITERAL("alice");
    LorStringView bob = LOR_SV_LITERAL("bob");
    lor_map_put_as(scores, Score, alice, 10);
    lor_map_put_as(scores, Score, bob, 8);
    lor_print("map", lor_print_map(scores));

    Point point = {3, 5};
    lor_print("custom", lor_print_custom(&point, print_point));

    Point *points = LOR_ARRAY_INIT;
    lor_array_push(points, point);
    lor_print("custom array", lor_print_array_with(points, print_point));

    LorArena arena = LOR_ARENA_INIT;
    lor_arena_alloc(&arena, 32);
    lor_print(arena);

    LorPrintConfig csv = {LOR_SV_LITERAL(", "), LOR_SV_LITERAL("\n")};
    lor_print_with(csv, "red", "green", "blue");

    lor_arena_deinit(&arena);
    lor_array_deinit(&points);
    lor_map_deinit(&scores);
    lor_set_deinit(&tags);
    lor_array_deinit(&numbers);
    lor_string_deinit(&owned);
    return 0;
}
