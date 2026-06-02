# liblor Roadmap

This file tracks current direction and the next concrete development steps.

## Current Focus

Stabilize the foundation after the first module.

1. Review `LorArena` after using it in examples.
2. Decide the project-wide allocator interface.
3. Define the initial error/result convention.
4. Choose the next small data module: string view or dynamic array.

The future build/run/REPL tool is a design target, but it should wait until the
library has allocator, error, string/path, dynamic array, and process helpers.

## Done

- [x] Bootstrap README, AGENTS, ROADMAP, MIT license, and `.gitignore`.
- [x] Move reference material under `references/`.
- [x] Add `THIRD_PARTY.md` for audit tracking.
- [x] Choose normal multi-file source layout: `include/lor/` plus `src/`.
- [x] Add Makefile-based local build/test workflow.
- [x] Implement first module: `LorArena`.
- [x] Add focused arena tests and a small arena example.
- [x] Add `docs/build-system.md` for the future `lor` tool direction.

## Open Decisions

- Allocator interface for data structures.
- Error/result convention.
- Reusable test harness shape.
- Leak-checking strategy.
- Next module: string view or dynamic array.
- Whether and when to generate a single-header distribution.

## Backlog

### Foundation

- [x] arena allocator;
- [ ] allocator interface;
- [ ] error/result conventions;
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
- [ ] optional single-header generation.

### Platform

- [ ] mmap abstraction;
- [ ] concurrency primitives;
- [ ] custom entry-point helpers.

## Session Checklist

1. Read `README.md`, `AGENTS.md`, and this file.
2. Check `git status --short`.
3. Pick the next item from `Current Focus`.
4. Audit reference material before copying or closely adapting code.
5. Run `make all` and update this file after finishing a step.
