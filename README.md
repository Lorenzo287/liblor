# liblor

liblor is a personal C library for bringing higher-level programming tools to C.
The goal is a cohesive set of small, readable, portable utilities for everyday C code.

## Modules

- [Memory](docs/memory.md): arenas, scratch scopes, mmap, cleanup helpers, and opt-in leak checking.
- [Strings](docs/string.md): borrowed string views and owned dynamic strings.
- [Dynamic Arrays](docs/array.md): typed-pointer dynamic arrays with checked growth.
- [Hash Maps](docs/map.md): typed hash maps with configurable key ownership.
- [Sets](docs/set.md): typed hash sets with mathematical operations.
- [Numeric Helpers](docs/numeric.md): single-evaluation minimum, maximum, and clamp helpers.
- [Generic Printing](docs/print.md): type-directed scalar, liblor object, and container printing.
- [Type Helpers](docs/type.md): C11 names for built-in and concrete liblor value types.
- [Random Numbers](docs/random.md): explicit-state PCG32 generation and system entropy.
- [Command-Line Parsing](docs/cli.md): function-based command-line parsing with generated help.
- [Concurrency](docs/concurrency.md): native threads, task groups, synchronization, and channels.
- [Tracing](docs/trace.md): buffered manual tracing and optional compiler-driven function tracing.

Internals:

- `lor/features.h`: compiler capability checks used by other modules.
- `lor/status.h`: small shared failure statuses.

## Single Header

liblor is a normal multi-file library first. Public headers live under
`include/lor/` so users can add `include/` to their compiler path and write
namespaced includes such as `#include "lor/memory.h"` to avoid collisions.
A generated single-header release is present as an alternative.

Generate `lor.h` with:

```powershell
make single-header
```

Basic use:

```c
#define LOR_IMPLEMENTATION
#include "lor.h"
```

Optional module and alias controls:

- `LOR_STRIP_PREFIX`: add aliases such as `arena_alloc`.
- `LOR_LEAKCHECK`: development build mode for location-aware leak checking
  across liblor memory calls and stdlib heap calls. It automatically includes
  the memory module when selective module macros are used.
- `LOR_ENABLE_<MODULE>`: include only a module and its dependencies. If only
  `LOR_IMPLEMENTATION` is defined all modules are included by default.

## Build And Test

```powershell
make
make check
make release
```

Build output is written under `.build/`. See [Building](docs/BUILD.md) for
tests, examples, compiler profiles, release artifacts, LTO, and single-header
generation.

## Conventions

- `lor_` for public functions.
- `LorName` for public types.
- `LOR_NAME` for public constants and feature macros.

The lor prefix is strippable in the single header version.
See [API Conventions](docs/API-CONVENTIONS.md) for additional information
about ownership, lifetime, failure.

## Compatibility

liblor targets C11 on Windows and Unix-like systems. Most of the API is
compiler-independent. GCC or Clang in C11-or-newer mode supports the complete
convenience API; automatic whole-function tracing is supported but remains
opt-in. For other C configurations, only the exceptions below need attention.

### Missing Convenience Features

| Configuration | What is not available |
| --- | --- |
| Native MSVC 19.39 or newer in C11 mode | Array, map, and set `_auto` operations; `LOR_AUTO_*` cleanup; automatic trace scopes and whole-function tracing. |
| Earlier native MSVC in C11 mode | Everything missing above, plus `lor_typeof` and inferred `lor_print_map`. |
| TCC | Generic `lor_print` and its container wrappers; `LOR_AUTO_*` cleanup; automatic trace scopes and whole-function tracing. |
| Other C11 or C17 compilers without extensions | `lor_typeof`; array, map, and set `_auto` operations; inferred `lor_print_map`; `LOR_AUTO_*` cleanup; automatic trace scopes and whole-function tracing. |
| C23 compilers without extensions | Array, map, and set `_auto` operations; `LOR_AUTO_*` cleanup; automatic trace scopes and whole-function tracing. |

The portable replacements are straightforward:

- replace container `_auto` operations with `_as` forms or named variables;
- name types explicitly and use `lor_print_map_as` instead of inferred forms;
- replace generic printing with the explicit `LorPrintValue` and
  `lor_fprint_values` API;
- call the matching release function instead of relying on `LOR_AUTO_*`;
- use manual trace begin/end calls when automatic tracing is unavailable.

C99 is supported only for consumer code linked to a separately built C11
library. It retains the ordinary APIs and container `_as` forms, but not the
C11 generic type and printing conveniences on a strictly conforming compiler.
Compiler extensions may make additional conveniences available.

The module documentation names the relevant `LOR_HAS_*` check for code that
needs conditional compilation. `lor/features.h` contains the shared compiler
detection.

### C++

Public headers provide `extern "C"` guards so C++ programs can link to a
library compiled as C. The ordinary function API and typed lvalue macros are
available, but conveniences based on C compound literals, generic selection,
or type inference are not exposed to C++. Compiler-specific cleanup and trace
helpers retain their own feature checks. There is no native C++ wrapper API.
Compile the single-header implementation in a C translation unit rather than
defining `LOR_IMPLEMENTATION` in C++ code.

## License

liblor is licensed under the MIT License. See
[Acknowledgements](docs/ACKNOWLEDGEMENTS.md) for the projects and developers
whose published work inspired its design.

Start with [Roadmap](docs/ROADMAP.md) when continuing development.
