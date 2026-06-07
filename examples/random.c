// SPDX-License-Identifier: MIT

#include "lor/random.h"

#include <stdio.h>

int main(void) {
    LorRandom random = LOR_RANDOM_INIT;
    if (lor_random_seed_system(&random) != LOR_STATUS_OK) {
        fputs("could not obtain system entropy\n", stderr);
        return 1;
    }

    fputs("dice:", stdout);
    for (size_t i = 0; i < 8; ++i)
        printf(" %u", lor_random_bounded_u32(&random, 6) + 1u);
    fputc('\n', stdout);

    printf("unit float: %.8f\n", (double)lor_random_f32(&random));
    printf("unit double: %.17g\n", lor_random_f64(&random));
    return 0;
}
