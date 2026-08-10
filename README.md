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

### Language And Compiler Features

liblor detects capabilities rather than relying only on compiler names. This
is the practical summary for C translation units. The complete library is
built and tested as C11; the C99 row describes consumer code using the public
headers with a separately built library, not a complete C99 implementation
build.

| C mode or compiler | Additional conveniences |
| --- | --- |
| C99 | Compound-literal `_as` forms for arrays, maps, and sets. |
| C11 or C17 | C99 conveniences plus `_Generic` type inspection, numeric helpers, and generic printing. |
| C23 | C11 conveniences plus standard `typeof`, `lor_typeof`, and inferred map printing. Automatic array, map, and set expressions still require statement expressions. |
| GCC or Clang | `__typeof__` and statement expressions enable the automatic array, map, set, and numeric forms. A C11-or-newer mode also enables the generic conveniences above. |
| TCC | Provides compound literals, `_Generic`, `typeof`, and statement expressions, so the automatic container and numeric forms are available. Generic printing is disabled because TCC cannot expand its deeply nested expression. |
| Native MSVC | A mode reporting C11 enables generic conveniences; version 19.39 or newer also provides `__typeof__`. MSVC has no statement expressions, so the automatic array, map, and set forms are unavailable. |

The capabilities map to library features and portable alternatives as follows:

| Required capability | Enabled library feature | Alternative when unavailable |
| --- | --- | --- |
| C99 compound literals (`LOR_HAS_COMPOUND_LITERALS`) | Array `push_as`/`insert_as` and map/set `_as` operations. | Pass a named lvalue to the ordinary typed macro, or call the corresponding `_raw` function. |
| C11 `_Generic` (`LOR_HAS_GENERIC_SELECTION`) | `lor_type_kind`/`lor_type_name`, `lor_min_as`/`lor_max_as`/`lor_clamp_as`, the C11 numeric dispatch path, and generic printing except on TCC. | Use an explicit `LorTypeKind`, the tagged-value printing API, or normal C expressions. The numeric macro layer is unavailable if neither generic selection nor the compiler-extension path is present. |
| `typeof` (`LOR_HAS_TYPEOF`) | `lor_typeof`; when generic printing is also available, inferred `lor_print_map`/`lor_print_map_custom`. | Write the type explicitly and, with generic printing, use `lor_print_map_as` or `lor_print_map_as_custom`. |
| `typeof` plus statement expressions (`LOR_HAS_STATEMENT_EXPRESSIONS`) | `lor_array_push_auto`, map and set `_auto` operations, and the compiler-extension numeric path. | Use the C99 `_as` forms, named-lvalue forms, or raw functions. C11 numeric helpers remain available through `_Generic`. |

The final API-specific checks are `LOR_HAS_ARRAY_PUSH_AUTO`,
`LOR_HAS_MAP_AUTO`, `LOR_HAS_SET_AUTO`, `LOR_HAS_NUMERIC_AUTO`,
`LOR_HAS_NUMERIC_AS`, `LOR_HAS_GENERIC_PRINT`, and
`LOR_HAS_PRINT_MAP_AUTO`. Prefer these when conditionally using a convenience
API; the lower-level capability macros are declared in `lor/features.h`.

GCC and Clang also support liblor's `LOR_AUTO_*` scope cleanup and
`LOR_TRACE_SCOPE` helpers through `__attribute__((cleanup))`; explicit release
functions and trace begin/end calls are the portable alternatives. Their
`-finstrument-functions` support can additionally enable automatic function
tracing. These compiler-only conveniences do not affect availability of the
underlying memory, container, or tracing modules. Check
`LOR_CLEANUP_SUPPORTED`, `LOR_TRACE_SCOPE_SUPPORTED`, and
`LOR_TRACE_AUTO_SUPPORTED` before depending on them.

Native MSVC and Windows TCC represent `long double` as `double`, so generic
type inspection, numeric dispatch, and printing treat them as the same type on
those targets.

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
