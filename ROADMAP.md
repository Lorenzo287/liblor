# liblor Roadmap

This file is the first stop for a new coding session. It tracks what has been
decided, what is still open, and the next concrete step.

## Current Focus

Stabilize the foundation after the first module.

Immediate next step:

1. Review `LorArena` after using it in one or two examples.
2. Decide the project-wide allocator interface for future data structures.
3. Define the initial error/result convention.
4. Keep the build/run/REPL idea as a design target, but do not implement it
   until the library foundation is ready.
5. Decide whether the next module should be string view or dynamic array.

## Progress

- [x] Create public README.
- [x] Create agent manual.
- [x] Create initial roadmap.
- [x] Choose project license: MIT.
- [x] Add attribution/license tracking file.
- [x] Choose initial source layout: normal library first.
- [x] Add minimal build system.
- [x] Add first public header.
- [x] Add first tests.
- [x] Implement first module: `LorArena`.
- [x] Move reference material under `references/`.
- [x] Add build/run/REPL design note.

## Open Decisions

### Project license

Decision: liblor-owned code is MIT-licensed.

Important: choosing MIT for liblor does not remove obligations from copied or
adapted third-party code. External code still needs attribution and license text
when required. For new liblor-owned files, prefer a short SPDX header such as
`// SPDX-License-Identifier: MIT` instead of copying the full license block into
every file.

### Source shape

Decision: normal source tree first.

Current layout:

```text
dev.c
THIRD_PARTY.md
docs/
  build-system.md
include/
  lor/
    lor.h
    arena.h
src/
  arena.c
tests/
  test_arena.c
examples/
  arena_basic.c
```

`include/lor/lor.h` should be the umbrella header. Module headers should remain
usable on their own when practical.

An optional generated single-header distribution can be added later if it proves
useful.

### Build system

Decision: use `dev.c` as a tiny helper for developing liblor itself. This is
not the same as the future liblor build/run tooling.

Current PowerShell workflow:

```powershell
clang -std=c11 -Wall -Wextra -Wpedantic dev.c -o liblor-dev.exe
.\liblor-dev.exe all
```

The helper defaults to Clang and honors `CC`, for example `$env:CC = 'gcc'`.
Keep the layout simple enough that CMake or an amalgamation generator can be
added later without replacing the source tree.

The future build-system component is a separate product direction: a `lor`
command that can run, build, test, and possibly provide a compile-backed C REPL
for projects using liblor. See `docs/build-system.md`.

## Module Backlog

### Foundation

- [x] arena allocator;
- [ ] allocator interface for future data structures;
- [ ] error/result conventions;
- [x] focused arena test executable;
- [ ] reusable test harness;
- [ ] leak-checking strategy;
- [x] local development helper.

### Data

- [ ] string view;
- [ ] owned dynamic string;
- [ ] dynamic array;
- [ ] hash map;
- [ ] random-number helpers.

### Ergonomics

- [ ] defer helper;
- [ ] automatic cleanup helper;
- [ ] CLI argument parser;
- [ ] generic print helpers;
- [ ] type helper macros;
- [ ] default-parameter experiments.
- [ ] `lor run` / `lor build` design and prototype;
- [ ] compile-backed C REPL design and prototype.

### Platform

- [ ] mmap abstraction;
- [ ] concurrency primitives;
- [ ] custom entry-point helpers.

## Inspiration Inventory

This inventory is not a license clearance. It is a reminder to audit before
copying or adapting code.

`THIRD_PARTY.md` is the source of truth for attribution and license tracking.

- `references/arena/tsoding/`: MIT-licensed arena inspiration.
- `references/build/nob/`: dual MIT/public-domain build helper inspiration.
- `references/strings/sds/`: BSD-style SDS string library by Salvatore
  Sanfilippo.
- `references/strings/string_view/`: MIT-licensed string-view inspiration.
- `references/hash_table/tsoding/`: hash table inspiration.
- `references/hash_table/stb/` and `references/dynamic_array/stb_ds.h`:
  stb-style data-structure inspiration.
- `references/concurrency/libdill/`: permissive concurrency reference.
- `references/concurrency/libmill/`: concurrency reference; audit before use.
- `references/libCello/`: BSD-style higher-level C inspiration.
- `references/mman (mmap)/`: MIT-licensed Windows mmap reference.
- `references/rand/`: Apache-2.0 random-number reference.
- `references/stb_misc/stb.h`: large utility reference; read only for specific
  needs.
- Smaller experiment folders: inspect and document source/author if reused.

## Session Checklist

Use this checklist when continuing work:

1. Read `README.md`, `AGENTS.md`, and this file.
2. Check whether a Git repository exists with `git status --short`.
3. Pick the next unchecked item from `Current Focus` or `Progress`.
4. Audit any inspiration source before copying or closely adapting code.
5. Update this roadmap after completing a step or making a decision.

## Next Implementation Plan

1. Decide how generic allocation should look across liblor.
2. Decide the initial error/result convention.
3. Refine `LorArena` if the allocator decision exposes API issues.
4. Keep `docs/build-system.md` updated as build/run/REPL decisions sharpen.
5. Pick and implement the next small data module, likely string view or dynamic
   array.
