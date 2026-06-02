# libCello Project Context

Cello is a library that brings high-level programming paradigms to C, including generic data structures, polymorphism, interfaces, constructors/destructors, and optional garbage collection. It operates as a modern runtime system for C.

## Project Structure

- `src/`: Implementation of core Cello types and interfaces (e.g., `Array.c`, `Table.c`, `GC.c`).
- `include/`: Contains `Cello.h`, the main header file for the library.
- `tests/`: Unit tests using a custom `ptest` framework.
- `examples/`: Example programs demonstrating library usage.
- `obj/`: Directory for compiled object files.

## Building and Running

The project uses a standard `Makefile`.

- **Build Library**: `make`
  Generates `libCello.a` (static) and `libCello.so` / `libCello.dll` (dynamic).
- **Run Tests**: `make check`
  Compiles and runs the test suite located in `tests/test.c`.
- **Build Examples**: `make examples`
  Compiles the example programs in the `examples/` directory.
- **Clean**: `make clean`
  Removes build artifacts.
- **Install**: `make install`
  Installs the library and headers (default `PREFIX` is `/usr/local`).

## Development Conventions

### Coding Style
- **Language**: C (specifically `gnu99` standard).
- **Types**: Uses `var` (usually `void*` or a typedef) to represent Cello objects.
- **Allocation**:
  - Stack allocation: `var x = $(Int, 5);`
  - Heap allocation: `var x = new(Array, Int, $I(1), $I(2));`
- **Interfaces**: Cello uses a "Type Class" system. Polymorphic functions like `get`, `set`, `len`, `push`, etc., are used across different collection types.
- **Naming**: Follows PascalCase for Types (e.g., `Array`, `Table`, `Int`) and snake_case for functions (e.g., `type_of`, `get_child`).

### Testing Practices
- Tests are written in `tests/test.c` using the `PT_FUNC` and `PT_ASSERT` macros from `ptest.h`.
- New features should include corresponding tests in `tests/test.c`.
- To run a specific test, you may need to modify `tests/test.c` or use the `ptest` command-line arguments if supported (check `tests/ptest.h`).

### Architecture Insights
- **Fat Pointers**: Cello relies on "fat pointers" and metadata associated with objects to provide runtime type information.
- **Garbage Collection**: Cello provides an optional mark-and-sweep garbage collector (see `src/GC.c`).
- **Header-Only API**: Most users only need to `#include "Cello.h"`.
