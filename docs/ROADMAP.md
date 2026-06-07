# liblor Roadmap

This file tracks current direction and the next concrete development steps.

## Current Focus

Complete the remaining small library modules before starting build tooling.

1. Define a small portable type-helper layer, with compiler extensions kept
   behind feature macros and portable alternatives.
2. Build generic print helpers on the settled type-helper conventions.
3. Scope concurrency separately before implementation.
4. Leave the `lor` build/run tool and compile-backed REPL until the library
   modules are otherwise complete.

## Open Decisions

- Reusable test harness shape.
- Single-header generator hardening as more modules are added.
- Shared allocator customization versus heap-only owned containers.
- Generic print support for user-defined types without excessive macros.
- Concurrency scope: basic threading primitives versus structured
  concurrency, channels, and asynchronous I/O.

## Done

- [x] Bootstrap README, AGENTS, ROADMAP, MIT license, and `.gitignore`.
- [x] Move reference material under `references/`.
- [x] Add `docs/THIRD_PARTY.md` for audit tracking.
- [x] Choose normal multi-file source layout: `include/lor/` plus `src/`.
- [x] Add Makefile-based local build/test workflow.
- [x] Add `compile_flags.txt` for clangd include resolution.
- [x] Implement first module: `LorArena`.
- [x] Add focused arena tests and a small arena example.
- [x] Add `docs/build-system.md` for the future `lor` tool direction.
- [x] Add generated `lor.h` workflow.
- [x] Consolidate public API naming: `lor_module_action`, `LorName`, `LOR_NAME`.
- [x] Add memory subsystem implementation path.
- [x] Add consolidated memory module with arenas, mmap, cleanup helpers, and
      opt-in leak checking.
- [x] Simplify Makefile source, test, example, and header discovery.
- [x] Define initial ownership, allocator, and failure conventions.
- [x] Add shared `LorStatus` values.
- [x] Add borrowed string views and owned dynamic strings.
- [x] Add single-header module dependencies.
- [x] Add typed-pointer dynamic arrays with checked growth.
- [x] Add typed hash maps with byte, custom, borrowed-string, and owned-string
      key policies.
- [x] Add typed hash sets with map-backed storage and mathematical operations.
- [x] Add function-based CLI parsing with generated help and typed values.
- [x] Add explicit-state PCG32 generation, unbiased bounded values, and system
      entropy.
- [x] Add isolated development profiles and optimized static/shared library
      release builds.

## Backlog

### Foundation

- [x] arena allocator;
- [x] arena mark/rewind scopes;
- [x] scratch arenas;
- [x] mmap file mapping;
- [x] opt-in leak checking;
- [x] error/result conventions;
- [x] consolidated memory module;
- [x] cleanup helpers;
- [ ] reusable test harness;

### Data

- [x] string view;
- [x] owned dynamic string;
- [x] dynamic array;
- [x] hash map;
- [x] set;
- [x] random-number helpers.

### Ergonomics

- [x] automatic cleanup helper;
- [x] CLI argument parser;
- [ ] generic print helpers;
- [ ] type helper macros.

### Tooling

- [x] optional single-header generation.
- [ ] `lor run` / `lor build` design and prototype, after library modules;
- [ ] compile-backed C REPL design and prototype, after build tooling.

### Platform

- [x] mmap abstraction;
- [ ] public virtual-memory API, if common use cases justify it;
- [ ] concurrency design and scope;
- [ ] concurrency primitives, only after the design is settled;
- [ ] custom entry-point helpers.
