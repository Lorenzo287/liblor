# liblor

liblor is a personal C library for bringing higher-level programming tools to C.
The goal is a cohesive set of small, readable, portable utilities for everyday C code.

The project is early. APIs may change.

## Build And Test

Use the Makefile for local development:

```powershell
make all
make check
```

targets:

- `make test`: build and run tests.
- `make example`: build examples.
- `make clang`: build and test with Clang in `.build/clang/`.
- `make gcc`: build and test with GCC in `.build/gcc/`.
- `make strict`: run a warning-as-error Clang build.
- `make leakcheck`: run a strict build with `LOR_LEAKCHECK`.
- `make check`: run strict Clang, strict GCC, and leak-check builds.
- `make release`: build optimized static and shared libraries.
- `make release-check`: test the optimized build and produce release libraries.
- `make release-lto`: build a compiler-specific link-time-optimized release.
- `make release-lto-check`: test static and shared LTO library consumers.
- `make single-header`: generate `lor.h`.
- `make clean`: remove all generated files under `.build/`.

The default build uses `.build/default/`. Named configurations use their own
subdirectories, so changing compilers or diagnostics does not leave executables
or extra build directories in the repository root. Use `make -j all` for a
parallel local build when needed.

Release builds use `-O3 -DNDEBUG` without CPU-specific flags, so the binaries
remain usable on machines other than the one that built them. The optional LTO
target performs additional whole-program optimization but is more tightly
coupled to its compiler toolchain. Artifacts are written under
`.build/release/<compiler>/`, with `-lto` appended for LTO builds:

- Windows with Clang: `lor_static.lib`, `lor.dll`, and import library `lor.lib`.
- Windows with GCC: `liblor.a`, `lor.dll`, and import library `liblor.dll.a`.
- Linux: `liblor.a` and `liblor.so`.
- macOS: `liblor.a` and `liblor.dylib`.

The compiled libraries contain every multi-file module implementation. Programs
still include the normal headers from `include/lor/`; the generated `lor.h` is
not involved. On Windows, DLL exports are generated from
`tools/lor_modules.json`, which remains the public-symbol source of truth.

## Layout

liblor is a normal multi-file library first. A generated single-header release
is present as an alternative.

Current modules include:

- `lor/array.h`: typed-pointer dynamic arrays with checked growth.
- `lor/cli.h`: function-based command-line parsing with generated help.
- `lor/map.h`: typed hash maps with configurable key ownership.
- `lor/memory.h`: arenas, scratch scopes, mmap, cleanup helpers, and opt-in
  leak checking.
- `lor/random.h`: explicit-state PCG32 generation and system entropy.
- `lor/set.h`: typed hash sets with Python-style mathematical operations.
- `lor/status.h`: small shared failure statuses.
- `lor/string.h`: borrowed string views and owned dynamic strings.

See [API Conventions](docs/api-conventions.md), [Command-Line Parsing](docs/cli.md),
[Dynamic Arrays](docs/array.md), [Hash Maps](docs/map.md),
[Memory](docs/memory.md), [Random Numbers](docs/random.md),
[Sets](docs/set.md), and [Strings](docs/string.md).

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

- `LOR_ENABLE_STATUS`: include only the status module.
- `LOR_ENABLE_MEMORY`: include only the memory module.
- `LOR_ENABLE_STRING`: include the string module and its status dependency.
- `LOR_ENABLE_ARRAY`: include the array module and its status dependency.
- `LOR_ENABLE_CLI`: include the CLI module and its array/string dependencies.
- `LOR_ENABLE_MAP`: include the map module and its string/status dependencies.
- `LOR_ENABLE_RANDOM`: include the random module and its status dependency.
- `LOR_ENABLE_SET`: include the set module and its map dependencies.
- `LOR_STRIP_PREFIX`: add aliases such as `arena_alloc`.
- `LOR_LEAKCHECK`: development build mode for location-aware leak checking
  across liblor memory calls and stdlib heap calls. It automatically includes
  the memory module when selective module macros are used.

## License

liblor-owned code is licensed under the MIT License. Reference material is not
automatically part of liblor; copied or closely adapted third-party code must be
audited and keep required notices. See [Third Party](./docs/THIRD_PARTY.md).

Start with [Roadmap](docs/ROADMAP.md) when continuing development.
