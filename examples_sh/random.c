#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_RANDOM
#include "../lor.h"

int main(void) {
    LorRandom state = LOR_RANDOM_INIT;
    // NOTE: if you don't specify a custom seed if defaults to
    // lor_random_seed(&state, 0, 0);

    // using bounded avoids the bias that comes from % 6
    int die = 1 + lor_random_bounded_u32(&state, 6);
    printf("die: %d\n", die);

    unsigned int seed = 12345;
    unsigned int stream = 0;
    lor_random_seed(&state, seed, stream);
    die = 1 + lor_random_bounded_u32(&state, 6);
    printf("die: %d\n", die);

	// NOTE: if instead you want a different result each run
	// you need to use the system entropy
    lor_random_seed_system(&state);
    float value = lor_random_f32(&state);
    printf("value: %f\n", value);
    return 0;
}
