# liblor Roadmap

This file tracks current direction and the next concrete development steps.

## Current Focus

Use the completed memory and container foundations to choose the next
practical module.

1. Exercise the containers in a larger feature before widening their APIs.
2. Choose between random-number helpers and a CLI argument parser.
3. Consider a reusable test harness when the next module adds repeated test
   infrastructure.

## Open Decisions

- Reusable test harness shape.
- Single-header generator hardening as more modules are added.
- Shared allocator customization versus heap-only owned containers.
- Next data or ergonomics module.

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
- [ ] random-number helpers.

### Ergonomics

- [x] automatic cleanup helper;
- [ ] CLI argument parser;
- [ ] generic print helpers;
- [ ] type helper macros.

### Tooling

- [ ] `lor run` / `lor build` design and prototype;
- [ ] compile-backed C REPL design and prototype;
- [x] optional single-header generation.

### Platform

- [x] mmap abstraction;
- [ ] public virtual-memory API, if common use cases justify it;
- [ ] concurrency primitives;
- [ ] custom entry-point helpers.
