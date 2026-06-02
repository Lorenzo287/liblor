# C Code Cleanup Examples

## Project Overview

This project contains a collection of C files that demonstrate different techniques for resource management and error handling in C, specifically for a file copying task. The main goal is to compare a "classic" manual approach with `goto`-based cleanup and a more modern approach using GCC's `__attribute__((cleanup))`.

The project also integrates `stb_leakcheck.h`, a single-header memory leak checker, to ensure that all implementations properly deallocate memory.

### Key Files

*   `copy_classic.c`: A standard implementation of file copy with manual error handling and resource cleanup.
*   `copy_goto.c`: An implementation that uses `goto` statements for centralized cleanup, a common C idiom.
*   `copy_cleanup.c`: An implementation that uses the `__attribute__((cleanup))` GCC extension for automatic resource cleanup when variables go out of scope. This provides a mechanism similar to RAII in C++.
*   `cleanup.h`: A header file that defines macros to simplify the usage of the `__attribute__((cleanup))` feature for file descriptors and memory.
*   `stb_leakcheck.h`: A public domain, single-header library for runtime memory leak detection in C/C++.

## Building and Running

The project does not contain a makefile. To compile and run the examples, you will need a C compiler like GCC or Clang.

### Example Compilation (using GCC)

```bash
# To compile and run the "classic" version
gcc copy_classic.c -o copy_classic.exe
./copy_classic.exe

# To compile and run the "goto" version
gcc copy_goto.c -o copy_goto.exe
./copy_goto.exe

# To compile and run the "cleanup" version (requires GCC or Clang)
gcc copy_cleanup.c -o copy_cleanup.exe
./copy_cleanup.exe
```

After running each executable, it will create a file named `copy_of_copy.txt` and print any memory leak information to the console.

## Development Conventions

*   **Error Handling**: The project explicitly explores different error handling patterns.
*   **Resource Management**: The core focus is on demonstrating different ways to manage resources like file handles and dynamic memory.
*   **Memory Safety**: All source files include `stb_leakcheck.h` to check for memory leaks. The `main` function in each file calls `stb_leakcheck_dumpmem()` before exiting.
*   **Compiler Features**: The `copy_cleanup.c` example relies on a non-standard GCC/Clang extension (`__attribute__((cleanup))`), making it less portable than the other examples.
