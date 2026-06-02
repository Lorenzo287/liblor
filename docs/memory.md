# Memory

Memory is the first liblor subsystem. Keep arenas, scratch scopes, virtual
memory, mmap, cleanup helpers, and leak checking together in
`include/lor/memory.h` and `src/memory.c` until the code clearly needs a split.

## Current Shape

- `LorArena`: heap-backed by default, with optional virtual-memory backend.
- `LorArenaMark` / `LorArenaTemp`: rewind temporary allocations.
- `lor_scratch_begin`: per-thread temporary scratch arenas.
- `LorVirtualMemory`: anonymous reserve/commit/release memory.
- `LorMmap`: file mapping by path.
- `lor_malloc` / `lor_free` helpers: present for leakcheck, not as a generic
  allocator abstraction.
- `LOR_LEAKCHECK_STDLIB`: optional macro mode for stdlib heap calls.

There is no public `LorAllocator`. Reintroduce an allocator interface only when
a concrete container or subsystem needs user-supplied allocation behavior.

## Principles

- Prefer useful features over wrapper APIs.
- Keep ownership explicit in names or docs.
- Prefer deterministic cleanup over hidden global behavior.
- Do not replace `malloc` globally unless the user opts into macro mode.
- Treat arena allocations as bulk-owned by the arena, not individually freeable.
- Do not attempt generic GC for arbitrary C pointers.

## Leak Checking

Leakcheck is off by default. When enabled with `lor_leakcheck_enable(1)`, liblor
tracks allocations made through liblor heap helpers, active arenas, virtual
reservations, and active mmap mappings.

For stdlib heap calls in one translation unit:

```c
#define LOR_LEAKCHECK_STDLIB
#include "lor/memory.h"
```

Then `malloc`, `calloc`, `realloc`, `free`, and `strdup` route through liblor's
tracker in that translation unit.

## Next Work

- Harden Unix virtual-memory behavior on a Unix host.
- Decide the first container memory policy when dynamic arrays/hash maps begin.
- Add richer leak reports only if the current report format is insufficient.
