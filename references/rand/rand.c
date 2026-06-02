/*
 * === PSEUDO RANDOM NUMBER GENERATOR ===
 *
 * Copyright (c) 2026 Lorenzo Tumini. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Portions of this software are derived from the PCG Random Number Generation
 * library, Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>. Licensed
 * under the Apache License, Version 2.0.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 * https://www.pcg-random.org/
 */

/*
 * For additional information about the Box-Muller transform implementation for
 * normal distribution generation visit
 * https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
 */

#include <math.h>
#include <stdio.h>
#include "rand.h"

int main(void) {
    // Obtain cryptographically secure random seeds from the operating system.
    uint64_t seeds[2] = {0};
    plat_get_entropy(seeds, sizeof(seeds));

    // Initialize the PRNG state with the obtained seeds.
    prng_state rng = {};
    prng_seed_r(&rng, seeds[0], seeds[1]);

    // Print 10 normally distributed random numbers.
    for (uint32_t i = 0; i < 10; i++) printf("%f ", prng_rand_norm_r(&rng));
    printf("\n");
    return 0;
}

static prng_state s_prng_state = {0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL,
                                  NAN};

// Seeds the PRNG with initial state and sequence.
void prng_seed_r(prng_state *rng, uint64_t initstate, uint64_t initseq) {
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    prng_rand_r(rng);
    rng->state += initstate;
    prng_rand_r(rng);
    rng->prev_norm = NAN;
}

// Seeds the global PRNG state.
void prng_seed(uint64_t initstate, uint64_t initseq) {
    prng_seed_r(&s_prng_state, initstate, initseq);
}

// Generates a 32-bit pseudo-random number using PCG algorithm.
uint32_t prng_rand_r(prng_state *rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

// Generates a 32-bit pseudo-random number using the global PRNG state.
uint32_t prng_rand(void) {
    return prng_rand_r(&s_prng_state);
}

// Generates a float pseudo-random number in the range [0, 1).
float prng_randf_r(prng_state *rng) {
    return (float)prng_rand_r(rng) / (float)UINT32_MAX;
}

// Generates a float pseudo-random number in the range [0, 1) using the global
// PRNG state.
float prng_randf(void) {
    return prng_randf_r(&s_prng_state);
}

// Generates a normally distributed pseudo-random number using the Box-Muller
// transform.
float prng_rand_norm_r(prng_state *rng) {
    if (!isnan(rng->prev_norm)) {
        float out = rng->prev_norm;
        rng->prev_norm = NAN;
        return out;
    }
    float u1 = 0.0f;
    do u1 = prng_randf_r(rng);
    while (u1 == 0.0f);
    float u2 = prng_randf_r(rng);

    float mag = sqrtf(-2.0f * logf(u1));

    float z0 = mag * cosf(2.0f * PIf * u2);
    float z1 = mag * sinf(2.0f * PIf * u2);

    rng->prev_norm = z1;
    return z0;
}

// Generates a normally distributed pseudo-random number using the global PRNG
// state.
float prng_rand_norm(void) {
    return prng_rand_norm_r(&s_prng_state);
}

#if defined(WIN32)
    #include <Windows.h>
    #include <bcrypt.h>
// Fills a buffer with cryptographically secure random bytes from the platform's
// entropy source.
void plat_get_entropy(void *data, uint32_t size) {
    BCryptGenRandom(NULL, data, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}
#elif defined(__linux__) || defined(__APPLE__)
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
void plat_get_entropy(void *data, uint32_t size) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        perror("open /dev/urandom");
        exit(1);
    }
    ssize_t result = read(fd, data, size);
    if (result < 0 || (size_t)result != size) {
        perror("read /dev/urandom");
        close(fd);
        exit(1);
    }
    close(fd);
}
#else
    #error "plat_get_entropy is not implemented for this platform"
#endif
