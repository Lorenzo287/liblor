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

This convention is provisional until the first real API lands.

## Planned Components

The current idea list includes:

- arena allocator;
- build system and integrated build helpers;
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

The current top-level folders are reference material and experiments:

- `arena/`: arena allocator ideas, including Tsoding-inspired code.
- `build/`: build-system ideas, including `nob`.
- `cleanup (auto free)/`: automatic cleanup and leak-checking experiments.
- `cli_args_parser/`: command-line parsing ideas.
- `concurrency/`: coroutine/concurrency references.
- `custom_main/`: custom entry-point experiments.
- `default_parameters/`: default-parameter macro experiments.
- `defer/`: defer-style cleanup snippets.
- `dynamic_array/`: dynamic array references and experiments.
- `error/`: error-reporting experiments.
- `generic (print + typeof)/`: generic printing and type-detection ideas.
- `hash_table/`: hash table references.
- `leakcheck/`: leak-checking experiments.
- `libCello/`: higher-level C programming inspiration.
- `mman (mmap)/`: Windows mmap compatibility reference.
- `rand/`: random-number utility reference.
- `stb_misc/`: large utility reference; read only when needed.
- `strings/`: string libraries and string-view references.
- `x_macro/`: X-macro experiments.

The intended future source layout is:

- `include/lor/`: public headers.
- `src/`: implementation files.
- `tests/`: focused tests for each module.
- `examples/`: small runnable examples.
- `tools/`: project-local tooling.
- `docs/`: design notes and deeper module documentation.

Whether liblor becomes a normal multi-file library, an `stb`-style single-header
library, or both is still open. The conservative target is a normal source tree
first, with an optional generated amalgamated header later if it proves useful.

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

An attribution file should be added before the first real import or rewrite from
external code. Expected credits include people and projects such as Antirez,
Tsoding, MagicalBait, stb, libCello, and any other source that influences the
library.

## Status

Bootstrap phase. No stable API exists yet.

Start with `ROADMAP.md` when continuing development.
