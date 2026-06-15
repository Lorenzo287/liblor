#include <stdio.h>

#define LOR_IMPLEMENTATION
#include "../lor.h"

#define SV(str) LOR_SV_LITERAL(str)

// NOTE: the struct must have this exact shape,
// there is dedicated macro to do the same
struct {
    LorStringView key;
    int value;
} x;

typedef LOR_MAP_ENTRY(LorStringView, int) map_entry;

int main(void) {
	assert(LOR_MAP_INIT == NULL);
	map_entry *map = LOR_MAP_INIT;

	LorStringView name = SV("Lorenzo");
	lor_map_put_as(map, map_entry, name, 21);
    return 0;
}
