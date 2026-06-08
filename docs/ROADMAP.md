# liblor Roadmap

This file records the completed version 1 scope and possible future work.

## Version 1

Version 1 is feature complete. Current work is limited to validation, bug fixes,
portability, and documentation for the existing module set.

## Done

- [x] Bootstrap README, AGENTS, ROADMAP, MIT license, and `.gitignore`.
- [x] Move reference material under `references/`.
- [x] Add `docs/THIRD_PARTY.md` for audit tracking.
- [x] Choose normal multi-file source layout: `include/lor/` plus `src/`.
- [x] Add Makefile-based local build/test workflow.
- [x] Add `compile_flags.txt` for clangd include resolution.
- [x] Implement first module: `LorArena`.
- [x] Add focused arena tests and a small arena example.
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
- [x] Add C11 names for built-in and concrete liblor value types.
- [x] Add generic tagged-value printing, custom endings, container formatting,
      and custom callbacks.
- [x] Audit and remove the STB utility header and Cello runtime experiments.
- [x] Add single-evaluation numeric minimum, maximum, and clamp helpers.
- [x] Consolidate repeated test assertions into a reusable internal header.
- [x] Define and implement the initial native-thread concurrency module.
- [x] Add structured task groups and buffered/unbuffered channels.
- [x] Keep build tooling and C REPL experiments outside liblor's scope.

## Version 1 Scope

### Foundation

- [x] arena allocator;
- [x] arena mark/rewind scopes;
- [x] scratch arenas;
- [x] mmap file mapping;
- [x] opt-in leak checking;
- [x] error/result conventions;
- [x] consolidated memory module;
- [x] cleanup helpers;
- [x] reusable test harness;

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
- [x] generic print helpers;
- [x] type helper macros.
- [x] numeric minimum, maximum, and clamp helpers;

### Tooling

- [x] optional single-header generation.

### Platform

- [x] mmap abstraction;
- [x] concurrency design and scope;
- [x] native concurrency primitives;
- [x] structured task groups and channels;

## Post-v1 Candidates

These are not commitments. Add them only when concrete programs demonstrate a
need:

- public virtual-memory API;
- shared allocator customization;
- channel `select`, worker pools, or asynchronous I/O;
- custom entry-point helpers;
- further single-header generator hardening.
