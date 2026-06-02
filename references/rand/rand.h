#include <stdint.h>

#define PI 3.14159265358979323846
#define PIf 3.14159265358979323846f

typedef struct {
    uint64_t state;   // Current state of the PRNG
    uint64_t inc;     // Incremental constant for the PRNG
    float prev_norm;  // Stores the second value from Box-Muller transform
} prng_state;

void prng_seed_r(prng_state *rng, uint64_t initstate, uint64_t initseq);
void prng_seed(uint64_t initstate, uint64_t initseq);

uint32_t prng_rand_r(prng_state *rng);
uint32_t prng_rand(void);

float prng_randf_r(prng_state *rng);
float prng_randf(void);

float prng_rand_norm_r(prng_state *rng);
float prng_rand_norm(void);

void plat_get_entropy(void *data, uint32_t size);
