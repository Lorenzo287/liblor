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

- `include/lor/`: public headers.
- `src/`: implementations.
- `tests/`: focused tests.
- `examples/`: small usage examples.
- `docs/`: design notes.
- `references/`: third-party libraries, snippets, and experiments for study.
- `tools/`: project tools, including the single-header generator.
- `lor.h`: generated single-header liblor.
- `docs/THIRD_PARTY.md`: attribution and license tracking.
- `compile_flags.txt`: clangd flags for resolving includes.

liblor is a normal multi-file library first. A generated single-header release
is present as an alternative.

Public headers live under `include/lor/` so users can add `include/` to their
compiler path and write namespaced includes such as `#include "lor/arena.h"`.
This avoids collisions with generic names like `arena.h`, `string.h`, or
`error.h`.

## Conventions

- `lor_` for public functions.
- `LorName` for public types.
- `LOR_NAME` for public constants and feature macros.

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

- `LOR_ENABLE_ARENA`: include only the arena module.
- `LOR_STRIP_PREFIX`: add aliases such as `arena_alloc`.
- `LOR_CUSTOM_PREFIX my_`: compile function symbols as `my_arena_alloc`, etc.

## License

liblor-owned code is licensed under the MIT License. Reference material is not
automatically part of liblor; copied or closely adapted third-party code must be
audited and keep required notices. See `docs/THIRD_PARTY.md`.

Start with `docs/ROADMAP.md` when continuing development.
