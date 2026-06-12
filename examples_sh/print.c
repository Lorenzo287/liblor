#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_PRINT
#include "../lor.h"

#define SV(str) LOR_SV_LITERAL(str)

typedef struct {
    int x;
    int y;
} Point;

LorStatus print_point(FILE *out, const void *val) {
    Point p = *(Point *)val;
    return fprintf(out, "<%d:%d>", p.x, p.y) < 0 ? LOR_STATUS_SYSTEM_ERROR
                                                 : LOR_STATUS_OK;
}

int main(void) {
    lor_print("=== GENERIC PRINT ===");
    // NOTE: lor_end() can be placed in any position
    lor_print(lor_end(""), "foo", 42, 3.14);
    lor_print(" bar", 'A', (char)'A', true);

    // can create a custom formatting
    LorPrintConfig csv = {.ending = SV("...\n"), .separator = SV("; ")};
    lor_print_with(csv, "ciao", "come", "va", "tutto", "bene");

    lor_print("\n=== LIBLOR TYPES ===");
    // liblor objects that have a dedicated type (everything but typed pointers
    // like array, map, set ecc) can be printed directly
    LorStringView sv = SV("arena type:");
    LorArena arena = {NULL, LOR_ARENA_BACKEND_HEAP, LOR_KIB(64), 0u, 0u};
	LorRandom state = LOR_RANDOM_INIT;
    lor_print(sv, arena, "\n", state);

    lor_print("\n=== COLLECTIONS ===");
    int *numbers = LOR_ARRAY_INIT;
    lor_array_push(numbers, (int){10});
    lor_array_push_as(numbers, int, 20);
    lor_array_push_auto(numbers, 30);

    typedef LOR_MAP_ENTRY(LorStringView, int) Score;
    Score *empty_map = LOR_MAP_INIT;

    int *empty_set = LOR_SET_INIT;

    // the array is simply a pointer to int, we cannot distinguis between a normal
    // pointer, array, map, set, ecc => must use a wrapper
    lor_print(lor_print_array(numbers));
    lor_print(lor_print_map(empty_map));
    lor_print(lor_print_set(empty_set));

    lor_print("\n=== CUSTOM TYPE ===");
    // must provide a callback in case of a custom type
    Point p = {3, 4};
    lor_print("point", lor_print_custom(&p, print_point));

    Point *points = LOR_ARRAY_INIT;
    lor_array_push(points, p);
    lor_array_push(points, ((Point){5, 7}));
    lor_print(lor_print_array_custom(points, print_point));

    return 0;
}
