# Build System Direction

This note is about the future liblor build/run tooling, not about `dev.c`.
`dev.c` is only a small helper for developing liblor itself.

## Goal

The long-term goal is to make small C projects feel closer to:

- `python main.py`: run the program without thinking about build output;
- `go run .`: compile, cache, and run in one command;
- `go build`: produce a final executable with predictable defaults.

C cannot truly behave like Python without either compiling or using an
interpreter/JIT. The practical target is a liblor command-line tool that hides
the compile/link/cache steps for normal projects.

## Possible Commands

Working name: `lor`.

```powershell
lor init
lor run .
lor run examples/hello.c
lor build
lor test
lor clean
lor repl
```

For simple projects, `lor run .` should work by convention: find source files,
include directories, compiler, output directory, and executable name. For
complex projects, the user should be able to provide an explicit C build script
or small config file.

## Relationship To nob

`nob` is a strong inspiration because it keeps build logic in C. The likely
shape is:

- default convention-based builder for simple projects;
- optional `lor.build.c` or similar for custom builds;
- reusable build API inside liblor for command execution, paths, files,
  dependency checks, and compiler invocation.

This means liblor can expose build-system utilities without forcing every
project to use the same build description.

## REPL Direction

A C REPL is possible, but it should be treated as a staged project.

The first practical version should be compile-backed:

1. keep a temporary generated C file for the session;
2. append declarations, helper functions, or expressions;
3. compile and run snippets through the selected compiler;
4. cache results where possible.

This is not a true Lisp-style live image. Persistent state and hot code reload
are harder in C. A later version could investigate dynamic-library loading,
TinyCC/libtcc, Clang tooling, or another JIT/interpreter approach, but those
choices need careful license and portability audits.

## Near-Term Plan

Do not build this before the library foundation is clearer. First define:

- allocator interface;
- error/result conventions;
- path/string utilities;
- dynamic arrays;
- process execution helpers.

Those modules are the natural substrate for a useful `lor` tool.
