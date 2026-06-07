# Random Numbers

`lor/random.h` provides explicit-state pseudorandom generation for simulations,
games, tests, procedural generation, and randomized algorithms. It uses the
PCG32 XSH-RR algorithm: a small, fast general-purpose generator with stable
cross-platform output for the same seed and stream.

PCG output is not cryptographically secure. Use
`lor_random_system_bytes` directly for keys, tokens, nonces, or other values
that must remain unpredictable.

## Deterministic State

`LorRandom` stores all mutable generator state:

```c
LorRandom random = LOR_RANDOM_INIT;
lor_random_seed(&random, 42, 54);

uint32_t value = lor_random_u32(&random);
```

The same seed and stream always produce the same sequence. Different streams
allow independent sequences without global mutable state. Separate
`LorRandom` objects may be used concurrently; the same object requires
external synchronization.

`LOR_RANDOM_INIT` is a valid fixed sequence equivalent to seed `0` and stream
`0`, so an initialized generator can be used immediately. Explicit seeding is
preferred when reproducibility matters.

## Values

- `lor_random_u32`: uniform full-range 32-bit value.
- `lor_random_u64`: two consecutive 32-bit outputs combined into one value.
- `lor_random_bounded_u32`: uniform value in `[0, bound)`.
- `lor_random_f32`: value in `[0, 1)` with 24 random bits.
- `lor_random_f64`: value in `[0, 1)` with 53 random bits.

Bounded generation uses rejection sampling rather than `% bound` alone, which
would bias results when the bound does not divide the full 32-bit range. A zero
bound returns zero without advancing the state.

The floating-point helpers discard excess integer bits before scaling. They
cannot produce `1.0`.

## System Entropy

Use operating-system entropy when each run should begin from a different
sequence:

```c
LorRandom random = LOR_RANDOM_INIT;
if (lor_random_seed_system(&random) != LOR_STATUS_OK) {
    // Handle unavailable system entropy.
}
```

`lor_random_seed_system` obtains two 64-bit seed values and leaves the existing
state unchanged on failure. Windows uses the UCRT's OS-backed `rand_s`; Unix-like
systems read `/dev/urandom` while handling interrupted and partial reads.

`lor_random_system_bytes` exposes that entropy source directly and returns
`LOR_STATUS_SYSTEM_ERROR` when the operating system call fails. Seeding PCG
from secure entropy makes the initial sequence difficult to guess, but it does
not turn later PCG output into cryptographically secure output.

See `examples/random.c`.
