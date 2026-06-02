# API Design

This note records consolidated API naming and generated-header direction.

## Function Names

Options considered:

- `lor_arena_alloc`: full snake case with project prefix and module name.
- `lorArenaAlloc`: camel case after the prefix.
- `lor_arenaAlloc`: mixed style.
- `arena_alloc`: module names only, no project prefix.
- `lorarenaalloc`: no separators.

Decision: keep `lor_module_action`, for example `lor_arena_alloc`.

Reasons:

- it is idiomatic and easy to scan in C;
- it groups naturally by module in autocomplete;
- it avoids collisions in user projects;
- it keeps generated aliases possible later;
- it stays readable when modules grow.

The way to reduce long names should be better word choice, not removing
separators. For example, prefer `lor_arena_reset` over a longer but more
precise name when the module context already explains the operation.

## Type Names

Options considered:

- `LorArena`;
- `struct lor_arena`;
- `lor_arena_t`;
- `Lor_Arena`.

Decision: use `LorArena` for public typedef names and simple
`struct LorArena { ... }` tags when the struct is public.

This is not a C language standard; it is a project convention. It keeps types
visually distinct from functions and macros. Avoid public `_t` names because
that suffix is reserved by POSIX for many implementation-defined type names.

## Prefixes And Aliases

Canonical liblor symbols should keep the `lor_` / `Lor` / `LOR_` prefixes.
That gives the library a stable ABI and avoids collisions.

Generated-header aliases are supported:

- `LOR_STRIP_PREFIX`: preprocessor aliases such as `arena_alloc` for
  `lor_arena_alloc`;
- `LOR_CUSTOM_PREFIX`: generated remapping for source-built or single-header
  users who want symbols such as `my_arena_alloc`.

Do not implement aliases manually in every module. `tools/gen_single_header.py`
generates the alias section from `tools/lor_modules.json` so it stays complete
and auditable.

Important distinction: strip-prefix aliases can be preprocessor-only while the
compiled symbols remain `lor_*`. Custom compiled symbol prefixes require source
or single-header builds with remapping before declarations and definitions.
They are not compatible with a prebuilt `liblor.a` unless that library was
compiled with the same prefix.

## Single Header

Keep normal source files as the development source of truth and generate
`dist/lor.h`.

Desired use:

```c
#define LOR_IMPLEMENTATION
#include "lor.h"
```

Module selection should also be generated. A practical policy:

- by default, `lor.h` exposes every stable module;
- if any `LOR_ENABLE_*` macro is defined, include only enabled modules;
- example: `#define LOR_ENABLE_ARENA` before including `lor.h`.

The generator should:

- concatenate selected public headers;
- strip local include guards and internal `#include "lor/..."` lines;
- append selected implementations under `#ifdef LOR_IMPLEMENTATION`;
- generate optional alias sections;
- preserve SPDX/license text and third-party notices when needed.

Do not hand-maintain the amalgamated header. Run `make single-header`.
