# liblor

liblor is a personal C library for bringing higher-level programming tools to C
without hiding the language. The goal is a cohesive set of small, readable,
portable utilities for everyday C code.

The project is early. APIs may change.

## Build And Test

Use the Makefile for local development:

```powershell
make all
```

To build with GCC:

```powershell
make CC=gcc all
```

Useful targets:

- `make test`: build and run tests.
- `make example`: build examples.
- `make format`: format liblor-owned C files with `.clang-format`.
- `make clean`: remove `.build/`.

If `make` is not available on Windows, `mingw32-make` can be used with the same
targets.

## Layout

- `include/lor/`: public headers.
- `src/`: implementations.
- `tests/`: focused tests.
- `examples/`: small usage examples.
- `docs/`: design notes.
- `references/`: third-party libraries, snippets, and experiments for study.
- `THIRD_PARTY.md`: attribution and license tracking.

Public headers live under `include/lor/` so users can add `include/` to their
compiler path and write namespaced includes such as `#include "lor/arena.h"`.
This avoids collisions with generic names like `arena.h`, `string.h`, or
`error.h`.

## Conventions

- `lor_` for public functions.
- `LorName` for public types.
- `LOR_NAME` for public constants and feature macros.
- `static` for file-local helpers.

liblor is a normal multi-file library first. A generated single-header release
may be added later if it proves useful.

## First Module

`LorArena` supports arena initialization, reset, deinitialization, aligned
allocation, zeroed allocation, array allocation, string duplication, and
usage/capacity inspection.

```c
#include "lor/lor.h"

LorArena arena = LOR_ARENA_INIT;
int *values = lor_arena_alloc_array_zero(&arena, 4, sizeof(*values));
char *label = lor_arena_strdup(&arena, "arena example");

lor_arena_deinit(&arena);
```

## License

liblor-owned code is licensed under the MIT License. Reference material is not
automatically part of liblor; copied or closely adapted third-party code must be
audited and keep required notices. See `THIRD_PARTY.md`.

Start with `ROADMAP.md` when continuing development.
