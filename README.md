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
compiler-independent; the table below covers optional conveniences that need
a newer C version or a compiler extension. Requirements in the table assume a
C translation unit. The C99 entry applies to consumer code linking a separately
built C11 library, not to a complete C99 implementation build.

### Feature Availability

| If you want to use | You need | If it is unavailable |
| --- | --- | --- |
| Inline container values, such as `lor_array_push_as`, `lor_map_put_as`, or `lor_set_add_as` | C99 or newer. | Put the value in a named variable and use `lor_array_push`, `lor_map_set`, or `lor_set_add`; the `_raw` functions are also available. |
| Inferred container expressions, such as `lor_array_push_auto` and map/set `_auto` operations | GCC, Clang, or TCC compiling C. | Use the `_as` forms or pass a named variable to the ordinary operation. |
| `lor_min`, `lor_max`, and `lor_clamp` | Any C11-or-newer compiler, or GCC, Clang, or TCC compiler extensions. The explicit-type `_as` variants require C11 or TCC. | Use an ordinary C comparison or a small application helper. |
| Automatic type names with `lor_type_kind` and `lor_type_name` | C11 or newer, or TCC. | Choose a `LorTypeKind` explicitly and pass it to `lor_type_kind_name`. |
| Type inference with `lor_typeof` | GCC, Clang, TCC, native MSVC 19.39 or newer, or a C23 compiler. | Write the type explicitly. |
| Generic `lor_print` calls | C11 or newer, except TCC. | Build `LorPrintValue` values explicitly and call `lor_fprint_values`. |
| Inferred map printing with `lor_print_map` | C11-or-newer GCC or Clang, native MSVC 19.39 or newer in C11 mode, or a C23 compiler. | Use `lor_print_map_as` to name the entry type. Without generic printing, use `lor_fprint_values`. |
| Automatic scope cleanup with `LOR_AUTO_FREE`, `LOR_AUTO_STRING`, `LOR_AUTO_ARRAY`, and the other `LOR_AUTO_*` declarations | GCC or Clang. | Call the matching release function explicitly, such as `free`, `lor_string_deinit`, or `lor_array_deinit`. |
| Automatic trace scopes or whole-function tracing | GCC or Clang. Whole-function tracing also requires `LOR_TRACE_AUTO` and `-finstrument-functions`. | Use `lor_trace_begin`/`lor_trace_end` and the other manual tracing calls. |

The underlying memory, string, container, printing, and tracing APIs remain
available when an optional convenience is not. Each optional API exposes a
feature check, such as `LOR_HAS_ARRAY_PUSH_AUTO`, `LOR_HAS_GENERIC_PRINT`, or
`LOR_CLEANUP_SUPPORTED`; module documentation gives the relevant check and
`lor/features.h` contains the shared compiler detection.

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
