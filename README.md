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

## Conventions

- `lor_` for public functions.
- `LorName` for public types.
- `LOR_NAME` for public constants and feature macros.

The lor prefix is strippable in the single header version.
See [API Conventions](docs/API-CONVENTIONS.md) for additional information
about ownership, lifetime, failure.

## Compatibility

liblor targets standard C on Windows and Unix-like systems. Public headers keep
C++ declaration and C ABI compatibility with `extern "C"` guards. Compile
liblor itself as C, then include its headers and link the resulting library
from C++. The ordinary function API and typed lvalue macros are available;
C-only conveniences based on compound literals, `_Generic`, `typeof`, or
statement expressions are not exposed to C++.

liblor does not currently provide a native C++ wrapper API. In particular,
compile the single-header implementation in a C translation unit rather than
defining `LOR_IMPLEMENTATION` in C++ code.

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

## License

liblor is licensed under the MIT License. See
[Acknowledgements](docs/ACKNOWLEDGEMENTS.md) for the projects and developers
whose published work inspired its design.

Start with [Roadmap](docs/ROADMAP.md) when continuing development.
