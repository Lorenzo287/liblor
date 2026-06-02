# Project Overview

This is a C project that uses the `libmill` library to implement Go-style concurrency. The main program in `main.c` demonstrates a simple producer-consumer pattern using coroutines and channels provided by `libmill`.

# Building and Running

## Prerequisites

*   A C compiler like `clang` or `gcc`.
*   The `libmill` library installed. The `Makefile` assumes it is installed in `~/opt/libmill`.

## Building

To build the project, you can use the provided `Makefile`.

```bash
make
```

This will compile `main.c` and create an executable named `main`. The command used is:

```bash
clang -Wall -Wextra -I$(HOME)/opt/libmill/include/ -o main main.c -L$(HOME)/opt/libmill/lib/ -l:libmill.a
```

Note: As per the comment in `main.c`, compilation might be intended to be done within a WSL (Windows Subsystem for Linux) environment if you are on Windows.

## Running

After a successful build, you can run the program:

```bash
./main
```

The program will output messages from the producer and consumer coroutines.

# Development Conventions

*   The code uses the `libmill` library for concurrency.
*   The project is built using a `Makefile`.
*   The code includes headers differently based on the operating system (`WIN32` vs. `__linux__`).
*   The `Makefile` uses `clang` with `-Wall` and `-Wextra` flags, indicating a preference for high warning levels.
