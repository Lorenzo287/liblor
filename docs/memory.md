# Memory

Memory is the first liblor subsystem. Keep arena allocation, allocator
interfaces, cleanup/defer, leak checking, and later ownership helpers together
in `include/lor/memory.h` and `src/memory.c` until the code clearly needs a
split.

## Current Status

- Implemented: `LorArena`, `LorAllocator`, and `lor_allocator_heap`.
- Public include: `#include "lor/memory.h"`.
- Single-header module macro: `LOR_ENABLE_MEMORY`.
- Next: settle arena/allocator compatibility, then add cleanup/defer.

## Principles

- Keep ownership explicit in names or docs.
- Prefer deterministic cleanup over hidden global behavior.
- Do not replace `malloc` globally.
- Keep debug memory tools opt-in.
- Do not attempt generic GC for arbitrary C pointers.

## Allocator Contract

`LorAllocator` stores a context pointer plus a realloc-style callback.

- `new_size == 0` frees and returns `NULL`.
- allocation failure returns `NULL` and leaves the old pointer valid.
- zero-count and overflowed array allocations return `NULL`.
- alignment is normal `malloc` alignment; over-aligned memory stays in APIs like
  `lor_arena_alloc_aligned`.

Arena allocation does not currently satisfy the full allocator contract because
arenas cannot free or reallocate individual allocations.

## Implementation Order

1. explicit allocation model;
2. deterministic cleanup/defer helpers;
3. opt-in debug leak checking;
4. optional reference counting when a real shared-ownership use case exists;
5. garbage collection only if a later object model justifies it.
