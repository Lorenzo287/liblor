// SPDX-License-Identifier: MIT

#include "lor/random.h"

#include <string.h>

#if defined(_WIN32)
#include <stdlib.h>

/* UCRT exposes rand_s only when _CRT_RAND_S is defined before stdlib.h.
   Generated all-module headers may include stdlib.h before this module, so
   declare its stable ABI directly instead of imposing a global feature macro. */
#if defined(__cplusplus)
extern "C" int __cdecl rand_s(unsigned int *value);
#else
extern int __cdecl rand_s(unsigned int *value);
#endif
#elif defined(__unix__) || defined(__APPLE__)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define LOR_RANDOM__PCG_MULTIPLIER UINT64_C(6364136223846793005)

uint32_t lor_random_u32(LorRandom *random) {
    if (random == NULL) return 0;

    uint64_t old_state = random->state;
    random->state = old_state * LOR_RANDOM__PCG_MULTIPLIER + random->increment;

    uint32_t xorshifted = (uint32_t)(((old_state >> 18u) ^ old_state) >> 27u);
    uint32_t rotation = (uint32_t)(old_state >> 59u);
    return (xorshifted >> rotation) | (xorshifted << ((0u - rotation) & 31u));
}

void lor_random_seed(LorRandom *random, uint64_t seed, uint64_t stream) {
    if (random == NULL) return;

    random->state = 0;
    random->increment = (stream << 1u) | UINT64_C(1);
    (void)lor_random_u32(random);
    random->state += seed;
    (void)lor_random_u32(random);
}

uint64_t lor_random_u64(LorRandom *random) {
    uint64_t high = lor_random_u32(random);
    uint64_t low = lor_random_u32(random);
    return (high << 32u) | low;
}

uint32_t lor_random_bounded_u32(LorRandom *random, uint32_t bound) {
    if (random == NULL || bound == 0) return 0;

    uint32_t threshold = (0u - bound) % bound;
    for (;;) {
        uint32_t value = lor_random_u32(random);
        if (value >= threshold) return value % bound;
    }
}

float lor_random_f32(LorRandom *random) {
    return (float)(lor_random_u32(random) >> 8u) / 16777216.0f;
}

double lor_random_f64(LorRandom *random) {
    return (double)(lor_random_u64(random) >> 11u) / 9007199254740992.0;
}

LorStatus lor_random_system_bytes(void *data, size_t size) {
    if (data == NULL && size != 0) return LOR_STATUS_INVALID_ARGUMENT;
    if (size == 0) return LOR_STATUS_OK;

#if defined(_WIN32)
    unsigned char *cursor = (unsigned char *)data;
    while (size != 0) {
        unsigned int value = 0;
        if (rand_s(&value) != 0) return LOR_STATUS_SYSTEM_ERROR;

        size_t chunk = size < sizeof(value) ? size : sizeof(value);
        memcpy(cursor, &value, chunk);
        cursor += chunk;
        size -= chunk;
    }
    return LOR_STATUS_OK;
#elif defined(__unix__) || defined(__APPLE__)
    int descriptor = open("/dev/urandom", O_RDONLY);
    if (descriptor < 0) return LOR_STATUS_SYSTEM_ERROR;

    unsigned char *cursor = (unsigned char *)data;
    size_t remaining = size;
    while (remaining != 0) {
        size_t chunk = remaining < 1024u * 1024u ? remaining : 1024u * 1024u;
        ssize_t count = read(descriptor, cursor, chunk);
        if (count > 0) {
            cursor += (size_t)count;
            remaining -= (size_t)count;
            continue;
        }
        if (count < 0 && errno == EINTR) continue;
        (void)close(descriptor);
        return LOR_STATUS_SYSTEM_ERROR;
    }

    if (close(descriptor) != 0) return LOR_STATUS_SYSTEM_ERROR;
    return LOR_STATUS_OK;
#else
    (void)data;
    return LOR_STATUS_SYSTEM_ERROR;
#endif
}

LorStatus lor_random_seed_system(LorRandom *random) {
    if (random == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    uint64_t seeds[2];
    LorStatus status = lor_random_system_bytes(seeds, sizeof(seeds));
    if (status != LOR_STATUS_OK) return status;

    lor_random_seed(random, seeds[0], seeds[1]);
    return LOR_STATUS_OK;
}
