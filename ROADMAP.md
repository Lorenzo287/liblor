# liblor Roadmap

This file is the first stop for a new coding session. It tracks what has been
decided, what is still open, and the next concrete step.

## Current Focus

Bootstrap the project into a maintainable library instead of a collection of
reference folders.

Immediate next step:

1. Decide the initial source layout and project license.
2. Create the first minimal liblor skeleton.
3. Implement one small foundational module with tests.

Recommended first module: arena allocator. It is foundational, small enough to
shape the style, and useful for later strings, arrays, maps, and parsing.

## Progress

- [x] Create public README.
- [x] Create agent manual.
- [x] Create initial roadmap.
- [ ] Choose project license.
- [ ] Add attribution/license tracking file.
- [ ] Choose final source layout.
- [ ] Add minimal build system.
- [ ] Add first public header.
- [ ] Add first tests.
- [ ] Implement first module.

## Open Decisions

### Project license

Target: permissive and usable almost everywhere.

Candidates to evaluate:

- MIT: familiar, widely accepted, compatible with many references.
- BSD-2-Clause or BSD-3-Clause: also permissive, matches some references.
- 0BSD: very permissive, but not all copied dependencies can be relicensed.

Important: choosing liblor's license does not remove obligations from copied or
adapted third-party code. External code still needs attribution and license text
when required.

### Source shape

Options:

- normal library: `include/lor/*.h` plus `src/*.c`;
- single-header library: `lor.h` with implementation guarded by
  `LOR_IMPLEMENTATION`;
- hybrid: normal source tree with generated amalgamated header.

Current recommendation: normal library first, optional generated single-header
later. This keeps development clean while leaving room for `stb`-style
distribution.

### Build system

Options:

- `nob`-style C build program;
- CMake;
- simple compiler scripts;
- hybrid local build helper plus generated project files.

Current recommendation: start with a tiny `nob`-style build helper because it is
close to the project's inspiration set and keeps the workflow C-native. Keep the
layout simple enough that CMake can be added later if needed.

## Proposed Initial Layout

Create this when the first implementation begins:

```text
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
tools/
docs/
```

`include/lor/lor.h` should be the umbrella header. Module headers should remain
usable on their own when practical.

## Module Backlog

### Foundation

- [ ] allocator interface and arena allocator;
- [ ] error/result conventions;
- [ ] test harness;
- [ ] leak-checking strategy;
- [ ] build helper.

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

### Platform

- [ ] mmap abstraction;
- [ ] concurrency primitives;
- [ ] custom entry-point helpers.

## Inspiration Inventory

This inventory is not a license clearance. It is a reminder to audit before
copying or adapting code.

- `arena/tsoding/`: MIT-licensed arena inspiration.
- `build/nob/`: dual MIT/public-domain build helper inspiration.
- `strings/sds/`: BSD-style SDS string library by Salvatore Sanfilippo.
- `strings/string_view/`: MIT-licensed string-view inspiration.
- `hash_table/tsoding/`: hash table inspiration.
- `hash_table/stb/` and `dynamic_array/stb_ds.h`: stb-style data-structure
  inspiration.
- `concurrency/libdill/`: permissive concurrency reference.
- `concurrency/libmill/`: concurrency reference; audit before use.
- `libCello/`: BSD-style higher-level C inspiration.
- `mman (mmap)/`: MIT-licensed Windows mmap reference.
- `rand/`: Apache-2.0 random-number reference.
- `stb_misc/stb.h`: large utility reference; read only for specific needs.
- Smaller experiment folders: inspect and document source/author if reused.

## Session Checklist

Use this checklist when continuing work:

1. Read `README.md`, `AGENTS.md`, and this file.
2. Check whether a Git repository exists with `git status --short`.
3. Pick the next unchecked item from `Current Focus` or `Progress`.
4. Audit any inspiration source before copying or closely adapting code.
5. Update this roadmap after completing a step or making a decision.

## First Implementation Plan

When ready to write code:

1. Choose the project license and create `LICENSE`.
2. Create `ATTRIBUTIONS.md` or `THIRD_PARTY.md`.
3. Create the proposed layout.
4. Add a minimal build command that works from PowerShell.
5. Implement `LorArena` with focused tests.
6. Add an example showing allocation, reset, and cleanup.
7. Update README with the first real usage example.
