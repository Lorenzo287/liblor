// SPDX-License-Identifier: MIT

#ifndef LOR_RANDOM_H
#define LOR_RANDOM_H

#include <stddef.h>
#include <stdint.h>

#include "lor/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Explicit PCG32 random state.

   Separate states may be used concurrently. The same mutable state requires
   external synchronization. This generator is intended for simulations,
   games, tests, and randomized algorithms, not cryptographic use. */
typedef struct LorRandom {
    // Current position in the selected PCG sequence.
    uint64_t state;
    // Encoded stream selector. Must remain odd.
    uint64_t increment;
} LorRandom;

/* A valid deterministic state equivalent to seed 0 and stream 0.

   Use `lor_random_seed` for a chosen reproducible sequence or
   `lor_random_seed_system` when each run should begin differently. */
#define LOR_RANDOM_INIT {UINT64_C(6364136223846793006), UINT64_C(1)}

// Selects a deterministic sequence from `seed` and `stream`.
void lor_random_seed(LorRandom *random, uint64_t seed, uint64_t stream);

/* Seeds `random` from the operating system's entropy source.

   The state is unchanged on failure. This does not make later PCG output
   cryptographically secure. */
LorStatus lor_random_seed_system(LorRandom *random);

/* Fills `data` with bytes from the operating system's entropy source.

   This operation is suitable when unpredictable bytes are required. Passing
   `NULL` is valid only when `size` is zero. */
LorStatus lor_random_system_bytes(void *data, size_t size);

// Returns the next uniformly distributed 32-bit value.
uint32_t lor_random_u32(LorRandom *random);

// Returns a 64-bit value composed from two consecutive 32-bit outputs.
uint64_t lor_random_u64(LorRandom *random);

/* Returns a uniformly distributed value in [0, bound).

   Rejection sampling avoids modulo bias. A zero bound returns zero without
   advancing the generator. */
uint32_t lor_random_bounded_u32(LorRandom *random, uint32_t bound);

// Returns a uniformly distributed float in [0, 1) with 24 random bits.
float lor_random_f32(LorRandom *random);

// Returns a uniformly distributed double in [0, 1) with 53 random bits.
double lor_random_f64(LorRandom *random);

#ifdef __cplusplus
}
#endif

#endif
