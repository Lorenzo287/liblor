# Build System Direction

This note is about the future liblor build/run tooling. It is separate from the
repository Makefile, which only builds liblor during development.

## Goal

Make small C projects feel closer to:

- `python main.py`: run without thinking about build output;
- `go run .`: compile, cache, and run in one command;
- `go build`: produce a final executable with predictable defaults.

C still has to compile or use an interpreter/JIT. The practical target is a
`lor` command that hides compile/link/cache details for common projects.

## Possible Commands

```powershell
lor init
lor run .
lor run examples/hello.c
lor build
lor test
lor clean
lor repl
```

Simple projects should work by convention. Complex projects can opt into a
small config file or C build script.

## Relationship To nob

`nob` is useful inspiration because it keeps build logic in C. A likely shape:

- convention-based defaults;
- optional `lor.build.c` for custom projects;
- reusable liblor APIs for paths, files, process execution, dependency checks,
  and compiler invocation.

## REPL Direction

The first practical C REPL should be compile-backed:

1. maintain a temporary generated C file for the session;
2. append declarations, helpers, or expressions;
3. compile and run snippets through the selected compiler;
4. cache results where possible.

A later version can investigate dynamic-library loading, TinyCC/libtcc, Clang
tooling, or another JIT/interpreter path. Those choices need license and
portability audits.

## Prerequisites

Do not build this before the library foundation is clearer. Needed first:

- allocator interface;
- error/result conventions;
- string/path utilities;
- dynamic arrays;
- process execution helpers.
