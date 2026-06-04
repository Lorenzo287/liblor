# liblor

liblor is a personal C library for bringing higher-level programming tools to C.
The goal is a cohesive set of small, readable, portable utilities for everyday C code.

The project is early. APIs may change.

## Build And Test

Use the Makefile for local development:

```powershell
make all
make CC=gcc all
```

targets:

- `make test`: build and run tests.
- `make example`: build examples.
- `make single-header`: generate `lor.h`.
- `make clean`: remove `.build/`.

## Layout

liblor is a normal multi-file library first. A generated single-header release
is present as an alternative.

The first implementation area is memory: arenas, scratch scopes, mmap, cleanup
helpers, and opt-in leak checking. Include `lor/memory.h` for the current memory
APIs. See [Memory](docs/memory.md).

Public headers live under `include/lor/` so users can add `include/` to their
compiler path and write namespaced includes such as `#include "lor/memory.h"`.
This avoids collisions with generic names like `memory.h`, `string.h`, or
`error.h`.

## Conventions

- `lor_` for public functions.
- `LorName` for public types.
- `LOR_NAME` for public constants and feature macros.

## Compatibility

liblor targets standard C on Windows and Unix-like systems. Public headers keep
C++ include compatibility with `extern "C"` guards, and platform-specific code
should stay isolated behind small `_WIN32` / Unix branches.

## Single Header

Generate `lor.h` with:

```powershell
make single-header
```

Basic use:

```c
#define LOR_IMPLEMENTATION
#include "lor.h"
```

Optional module and alias controls:

- `LOR_ENABLE_MEMORY`: include only the memory module.
- `LOR_STRIP_PREFIX`: add aliases such as `arena_alloc`.
- `LOR_CUSTOM_PREFIX my_`: compile function symbols as `my_arena_alloc`, etc.
- `LOR_LEAKCHECK`: development build mode for location-aware leak checking
  across liblor memory calls and stdlib heap calls.

## License

liblor-owned code is licensed under the MIT License. Reference material is not
automatically part of liblor; copied or closely adapted third-party code must be
audited and keep required notices. See [Third Party](./docs/THIRD_PARTY.md).

Start with [Roadmap](docs/ROADMAP.md) when continuing development.
