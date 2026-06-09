# liblor

liblor is a personal C library for bringing higher-level programming tools to C.
The goal is a cohesive set of small, readable, portable utilities for everyday C code.

## Layout

liblor is a normal multi-file library first. A generated single-header release
is present as an alternative.

Current modules include:

- `lor/memory.h`: arenas, scratch scopes, mmap, cleanup helpers, and opt-in leak checking.
- `lor/string.h`: borrowed string views and owned dynamic strings.
- `lor/array.h`: typed-pointer dynamic arrays with checked growth.
- `lor/map.h`: typed hash maps with configurable key ownership.
- `lor/set.h`: typed hash sets with Python-style mathematical operations.
- `lor/numeric.h`: single-evaluation minimum, maximum, and clamp helpers.
- `lor/print.h`: type-directed scalar, liblor object, and container printing.
- `lor/type.h`: C11 names for built-in and concrete liblor value types.
- `lor/random.h`: explicit-state PCG32 generation and system entropy.
- `lor/cli.h`: function-based command-line parsing with generated help.
- `lor/concurrency.h`: native threads, task groups, synchronization, and channels.
- `lor/features.h`: compiler capability checks used by optional conveniences.
- `lor/status.h`: small shared failure statuses.

See [API Conventions](docs/API-CONVENTIONS.md),
[Memory](docs/memory.md), [Strings](docs/string.md),
[Dynamic Arrays](docs/array.md), [Hash Maps](docs/map.md),
[Sets](docs/set.md), [Numeric Helpers](docs/numeric.md),
[Generic Printing](docs/print.md), [Type Helpers](docs/type.md),
[Random Numbers](docs/random.md), [Command-Line Parsing](docs/cli.md) 
and [Concurrency](docs/concurrency.md).

Public headers live under `include/lor/` so users can add `include/` to their
compiler path and write namespaced includes such as `#include "lor/memory.h"`.
This avoids collisions with generic names like `memory.h`, `string.h`, or
`error.h`.

## Conventions

- `lor_` for public functions.
- `LorName` for public types.
- `LOR_NAME` for public constants and feature macros.

## Compatibility

liblor targets standard C on Windows and Unix-like systems. Public headers keep
C++ include compatibility with `extern "C"` guards, and platform-specific code
should stay isolated behind small `_WIN32` / Unix branches.

## Single Header

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

- `LOR_ENABLE_STATUS`: include only the status module.
- `LOR_ENABLE_CONCURRENCY`: include concurrency and its status dependency.
- `LOR_ENABLE_FEATURES`: include compiler feature detection only.
- `LOR_ENABLE_TYPE`: include rich type inspection and its value-type dependencies.
- `LOR_ENABLE_MEMORY`: include only the memory module.
- `LOR_ENABLE_NUMERIC`: include numeric helpers and feature detection.
- `LOR_ENABLE_STRING`: include the string module and its status dependency.
- `LOR_ENABLE_PRINT`: include generic printing and supported container modules.
- `LOR_ENABLE_ARRAY`: include the array module and its status/features dependencies.
- `LOR_ENABLE_CLI`: include the CLI module and its array/string dependencies.
- `LOR_ENABLE_MAP`: include the map module and its string/features dependencies.
- `LOR_ENABLE_RANDOM`: include the random module and its status dependency.
- `LOR_ENABLE_SET`: include the set module and its map/features dependencies.

## Build And Test

```powershell
make all
make check
make release-check
```

Build output is written under `.build/`. See [Building](docs/BUILD.md) for
compiler profiles, release artifacts, LTO, and single-header generation.

## License

liblor is licensed under the MIT License. See
[Acknowledgements](docs/ACKNOWLEDGEMENTS.md) for the projects and developers
whose published work inspired its design.

Start with [Roadmap](docs/ROADMAP.md) when continuing development.
