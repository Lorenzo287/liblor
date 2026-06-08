// SPDX-License-Identifier: MIT

#include "lor/random.h"

#include <stdio.h>
#include <string.h>

#include "lor_test.h"

static int test_documented_pcg_sequence(void) {
    LorRandom random = LOR_RANDOM_INIT;
    lor_random_seed(&random, UINT64_C(42), UINT64_C(54));

    const uint32_t expected[] = {
        UINT32_C(0xa15c02b7), UINT32_C(0x7b47f409), UINT32_C(0xba1d3330),
        UINT32_C(0x83d2f293), UINT32_C(0xbfa4784b), UINT32_C(0xcbed606e),
    };
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        CHECK(lor_random_u32(&random) == expected[i]);
    return 0;
}

static int test_seed_and_initializer(void) {
    LorRandom initialized = LOR_RANDOM_INIT;
    LorRandom seeded = LOR_RANDOM_INIT;
    lor_random_seed(&seeded, 0, 0);

    for (size_t i = 0; i < 16; ++i)
        CHECK(lor_random_u32(&initialized) == lor_random_u32(&seeded));

    LorRandom a = LOR_RANDOM_INIT;
    LorRandom b = LOR_RANDOM_INIT;
    lor_random_seed(&a, 123, 1);
    lor_random_seed(&b, 123, 2);
    CHECK(lor_random_u32(&a) != lor_random_u32(&b));

    LorRandom unchanged = a;
    lor_random_seed(NULL, 1, 2);
    CHECK(lor_random_u32(NULL) == 0);
    CHECK(lor_random_u64(NULL) == 0);
    CHECK(lor_random_f32(NULL) == 0.0f);
    CHECK(lor_random_f64(NULL) == 0.0);
    CHECK(memcmp(&a, &unchanged, sizeof(a)) == 0);
    return 0;
}

static int test_u64_and_bounded_values(void) {
    LorRandom combined = LOR_RANDOM_INIT;
    LorRandom separate = LOR_RANDOM_INIT;
    lor_random_seed(&combined, 42, 54);
    lor_random_seed(&separate, 42, 54);

    uint64_t expected = (uint64_t)lor_random_u32(&separate) << 32u;
    expected |= lor_random_u32(&separate);
    CHECK(lor_random_u64(&combined) == expected);

    LorRandom random = LOR_RANDOM_INIT;
    uint64_t state = random.state;
    CHECK(lor_random_bounded_u32(&random, 0) == 0);
    CHECK(random.state == state);

    unsigned int seen[7] = {0};
    for (size_t i = 0; i < 10000; ++i) {
        uint32_t value = lor_random_bounded_u32(&random, 7);
        CHECK(value < 7);
        seen[value] += 1u;
        CHECK(lor_random_bounded_u32(&random, 1) == 0);
    }
    for (size_t i = 0; i < 7; ++i) CHECK(seen[i] != 0);
    return 0;
}

static int test_unit_values(void) {
    LorRandom random = LOR_RANDOM_INIT;
    for (size_t i = 0; i < 100000; ++i) {
        float value_f32 = lor_random_f32(&random);
        double value_f64 = lor_random_f64(&random);
        CHECK(value_f32 >= 0.0f && value_f32 < 1.0f);
        CHECK(value_f64 >= 0.0 && value_f64 < 1.0);
    }
    return 0;
}

static int test_system_entropy(void) {
    CHECK(lor_random_system_bytes(NULL, 0) == LOR_STATUS_OK);
    CHECK(lor_random_system_bytes(NULL, 1) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_random_seed_system(NULL) == LOR_STATUS_INVALID_ARGUMENT);

    unsigned char bytes[32];
    memset(bytes, 0xa5, sizeof(bytes));
    CHECK(lor_random_system_bytes(bytes, sizeof(bytes)) == LOR_STATUS_OK);

    int changed = 0;
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        if (bytes[i] != 0xa5) {
            changed = 1;
            break;
        }
    }
    CHECK(changed);

    LorRandom random = LOR_RANDOM_INIT;
    CHECK(lor_random_seed_system(&random) == LOR_STATUS_OK);
    CHECK((random.increment & UINT64_C(1)) != 0);
    return 0;
}

int main(void) {
    CHECK(test_documented_pcg_sequence() == 0);
    CHECK(test_seed_and_initializer() == 0);
    CHECK(test_u64_and_bounded_values() == 0);
    CHECK(test_unit_values() == 0);
    CHECK(test_system_entropy() == 0);

    puts("test_random: ok");
    return 0;
}
