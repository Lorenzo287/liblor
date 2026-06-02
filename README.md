# liblor

liblor is a personal C library for bringing higher-level programming tools to C
without hiding the language. The goal is a cohesive set of small, readable,
portable utilities that make everyday C code easier to write, debug, and
maintain.

The project is early and currently uses this repository as an inspiration
workbench. The folders in this tree contain existing libraries, experiments,
snippets, and notes that may inform liblor. Code should only become part of
liblor after it has been reviewed, adapted to the library style, tested, and
checked for license compatibility.

## Direction

liblor should feel like pragmatic C:

- minimal interfaces with predictable ownership rules;
- readable implementations with comments where they clarify intent;
- strong preference for explicit behavior over clever macros;
- consistent naming across every module;
- permissive open-source distribution, with clear attribution for inspirations
  and reused code.

The working public API convention is:

- `lor_` prefix for public functions;
- `LorName` for public types;
- `LOR_NAME` for public constants and feature macros;
- `lor_name` for private or internal helpers when they are not file-local;
- file-local helpers should be `static` and may use simple module-local names.

This convention is active for new liblor-owned code, but APIs may still change
while the project is young.

## Planned Components

The current idea list includes:

- arena allocator;
- project runner, build helper, and optional REPL tooling;
- automatic cleanup and defer-style helpers;
- command-line argument parsing;
- concurrency primitives;
- dynamic arrays;
- hash maps;
- string manipulation;
- error handling;
- generic print and type helpers;
- automatic leak checking;
- mmap support;
- better random numbers.

This list is not final and is not ordered by priority. See `ROADMAP.md` for the
current session plan.

## Repository Layout

Reference material and experiments live under `references/`:

- `references/arena/`: arena allocator ideas.
- `references/build/`: build-system ideas, including `nob`.
- `references/cleanup (auto free)/`: automatic cleanup and leak-checking
  experiments.
- `references/cli_args_parser/`: command-line parsing ideas.
- `references/concurrency/`: coroutine/concurrency references.
- `references/custom_main/`: custom entry-point experiments.
- `references/default_parameters/`: default-parameter macro experiments.
- `references/defer/`: defer-style cleanup snippets.
- `references/dynamic_array/`: dynamic array references and experiments.
- `references/error/`: error-reporting experiments.
- `references/generic (print + typeof)/`: generic printing and type-detection
  ideas.
- `references/hash_table/`: hash table references.
- `references/leakcheck/`: leak-checking experiments.
- `references/libCello/`: higher-level C programming inspiration.
- `references/mman (mmap)/`: Windows mmap compatibility reference.
- `references/rand/`: random-number utility reference.
- `references/stb_misc/`: large utility reference; read only when needed.
- `references/strings/`: string libraries and string-view references.
- `references/x_macro/`: X-macro experiments.

The liblor-owned source layout is:

- `dev.c`: tiny development helper for building and testing liblor itself.
- `include/lor/`: public headers.
- `src/`: implementation files.
- `tests/`: focused tests for each module.
- `examples/`: small runnable examples.
- `THIRD_PARTY.md`: license and inspiration tracking.

Expected future additions:

- `tools/`: project-local tooling.
- `docs/`: design notes and deeper module documentation.

liblor is a normal multi-file library first. An optional generated
`stb`-style amalgamated header may be added later if it proves useful.

Public headers live under `include/lor/` so users can add `include/` to their
compiler search path and write namespaced includes such as `#include
"lor/arena.h"`. This avoids collisions with generic names like `arena.h`,
`string.h`, or `error.h`, and it maps cleanly to future installation layouts.

## Build And Test

The current development helper expects Clang or GCC. From PowerShell:

```powershell
clang -std=c11 -Wall -Wextra -Wpedantic dev.c -o liblor-dev.exe
.\liblor-dev.exe all
```

To use GCC instead:

```powershell
$env:CC = 'gcc'
.\liblor-dev.exe all
Remove-Item Env:CC
```

Build outputs go into `.build/`.

The future liblor build/run/REPL feature is separate from this helper. See
`docs/build-system.md`.

## First Module

`LorArena` is the first liblor module. It supports arena initialization,
bulk reset, deinitialization, aligned allocation, zeroed allocation, array
allocation, string duplication, and usage/capacity inspection.

```c
#include "lor/lor.h"

LorArena arena = LOR_ARENA_INIT;
int *values = lor_arena_alloc_array_zero(&arena, 4, sizeof(*values));
char *label = lor_arena_strdup(&arena, "arena example");

lor_arena_deinit(&arena);
```

## Licensing And Attribution

liblor is licensed under the MIT License. The intent is permissive open-source
software that can be used almost anywhere.

Do not copy code from the reference folders into liblor until the source license
has been checked and recorded. Local references currently include MIT,
BSD-style, Apache-2.0, and public-domain or dual-license material. Compatible
code still needs attribution when required by its license and credit when it
meaningfully influenced the design.

New liblor-owned source files may use a short SPDX header instead of copying the
full MIT text into every file:

```c
// SPDX-License-Identifier: MIT
```

Copied or closely adapted third-party files are different: preserve the upstream
copyright and license notices required by that source.

See `THIRD_PARTY.md` for the current reference and attribution inventory.

## Status

Early development. The first module exists, but the API is not stable yet.

Start with `ROADMAP.md` when continuing development.
