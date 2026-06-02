# Project Overview

This is a C project that implements a pseudo-random number generator (PRNG). It uses the PCG algorithm for generating random numbers and the Box-Muller transform to generate normally distributed random numbers.

The main file is `rand.c`, which contains the PRNG implementation and a `main` function that demonstrates its usage.

# Building and Running

To build the project, you can use the following command:

Windows:

```sh
cc rand.c -lBcrypt -o rand
```

Linux/macOS:

```sh
cc rand.c -lm -o rand
```

To run the compiled program, execute the following command:

```sh
./rand
```
