#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_NUMERIC
#include "../lor.h"

int main(void) {
	int a = 500, b = 5;
	int c = lor_max(a, b);
	printf("max: %d\n", c);

	// can mix types, type is inferred from the type of (c + e)
	float e = 1000.2;
	printf("max mixed: %f\n", lor_max(c, e));

	// can choose an explicit cast
	printf("max forced cast: %d\n", lor_max_as(int, c, e));
	return 0;
}
