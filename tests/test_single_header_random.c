// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_RANDOM
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>
#include <string.h>

#include "lor_test.h"

int main(void) {
    Random random = RANDOM_INIT;
    random_seed(&random, 42, 54);
    CHECK(random_u32(&random) == UINT32_C(0xa15c02b7));
    CHECK(random_bounded_u32(&random, 6) < 6);
    CHECK(random_f64(&random) >= 0.0);

    unsigned char bytes[8];
    CHECK(random_system_bytes(bytes, sizeof(bytes)) == STATUS_OK);
    CHECK(strcmp(status_name(STATUS_SYSTEM_ERROR), "system error") == 0);

    puts("test_single_header_random: ok");
    return 0;
}
