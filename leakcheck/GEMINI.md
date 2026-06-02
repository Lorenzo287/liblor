# GEMINI.md

## Project Overview

This is a small C project demonstrating the usage of `stb_leakcheck.h`, a single-file public domain library for runtime memory leak detection in C/C++.

The main program in `main.c` allocates and frees a block of memory. By including `stb_leakcheck.h` and defining `STB_LEAKCHECK_IMPLEMENTATION`, the standard memory functions (`malloc`, `free`, `realloc`) are replaced with instrumented versions. The library is designed to automatically print a report of any unfreed memory allocations when the program exits.

## Building and Running

### Building

To build the project, you need a C compiler like GCC, Clang, or MSVC. The following command compiles `main.c` and creates an executable.

**Using GCC or Clang:**
```bash
gcc main.c -o main
```

**Using MSVC (in a developer command prompt):**
```bash
cl main.c
```

### Running

Execute the compiled program from your terminal.

```bash
./main
```

Or on Windows:
```bash
main.exe
```

The program will run and, upon exiting, `stb_leakcheck` will automatically report if any memory leaks were detected. Given the code in `main.c`, no leaks should be reported.

## Development Conventions

*   **Language:** The project is written in C.
*   **Memory Debugging:** `stb_leakcheck.h` is used for memory leak detection. To use it, you must `#define STB_LEAKCHECK_IMPLEMENTATION` in one C/C++ file before including the header.
*   **Style:** The code follows standard C conventions.
