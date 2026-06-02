# liblor Roadmap

This file tracks current direction and the next concrete development steps.

## Current Focus

Build the memory subsystem foundation. See `docs/memory.md`.

1. Decide how `LorArena` exposes or documents allocator compatibility.
2. Define the initial error/result convention for memory failures.
3. Add cleanup/defer primitives.
4. Add opt-in leak checking on top of `LorAllocator`.
5. Revisit reference counting only after a real shared-ownership use case exists.

Reference counting and garbage collection are later memory features. They need
real use cases before implementation.

## Open Decisions

- Arena allocator adapter decision.
- Error/result convention.
- Reusable test harness shape.
- Leak-checking strategy.
- Next module: string view or dynamic array.
- Single-header generator hardening as more modules are added.

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
- [x] Add consolidated memory module with arena and allocator foundation.
- [x] Simplify Makefile source, test, example, and header discovery.

## Backlog

### Foundation

- [x] arena allocator;
- [x] allocator interface and heap allocator;
- [ ] error/result conventions;
- [x] consolidated memory module;
- [ ] cleanup/defer scope;
- [ ] reusable test harness;
- [ ] leak-checking strategy.

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
- [ ] type helper macros.

### Tooling

- [ ] `lor run` / `lor build` design and prototype;
- [ ] compile-backed C REPL design and prototype;
- [x] optional single-header generation.

### Platform

- [ ] mmap abstraction;
- [ ] concurrency primitives;
- [ ] custom entry-point helpers.
