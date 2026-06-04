# Memory

Memory is the first liblor subsystem. Keep arenas, scratch scopes, mmap, cleanup
helpers, and leak checking together in
`include/lor/memory.h` and `src/memory.c` until the code clearly needs a split.

## Current Shape

- `LorArena`: heap-backed by default, with optional virtual-memory backend.
- `LorArenaMark` / `lor_arena_mark` / `lor_arena_rewind`: explicit
  checkpoints for rewinding temporary allocations inside the same arena.
- `LorScratch` / `lor_scratch_begin`: scope handles for per-thread scratch
  arenas.
- `LorMmap`: file mapping by path.
- cleanup helpers: scope-exit cleanup for heap pointers, arenas, scratch scopes,
  mmap mappings, and `FILE *` handles on compilers that support cleanup
  attributes.
- `LOR_LEAKCHECK`: development build mode for location-aware leak checking.

There is no public `LorAllocator`. Reintroduce an allocator interface only when
a concrete container or subsystem needs user-supplied allocation behavior.

The arena virtual-memory backend is an implementation detail. A public manual
virtual-memory API can be added later if common liblor use cases justify it.

## Principles

- Prefer useful features over wrapper APIs.
- Keep ownership explicit in names or docs.
- Prefer deterministic cleanup over hidden global behavior.
- Do not replace `malloc` globally unless the user opts into macro mode.
- Treat arena allocations as bulk-owned by the arena, not individually freeable.
- Do not attempt generic GC for arbitrary C pointers.

## Leak Checking

Leakcheck is a development build mode. Normal builds compile tracking out.
Define `LOR_LEAKCHECK` for the whole build so liblor memory calls and stdlib
heap calls route through location-aware tracking macros.

Single-header development build:

```c
#define LOR_IMPLEMENTATION
#define LOR_LEAKCHECK
#include "lor.h"
```

Multi-file development build:

```powershell
make CPPFLAGS="-Iinclude -DLOR_LEAKCHECK" all
```

For multi-file builds, compile `src/memory.c` with `LOR_LEAKCHECK` to enable
tracking and define it in consuming translation units to capture call-site file
and line. In practice, pass `-DLOR_LEAKCHECK` to the whole project build.

When active, leakcheck tracks liblor heap helpers, active arenas, virtual
arena lifetimes, and active mmap mappings. It does not own or clean up
resources; it only reports resources whose matching release/deinit/free/unmap
call was not made.

## Arenas, Temps, And Scratch

Use an arena when many allocations share one lifetime. Individual arena
allocations are not freed; the whole arena is reset, rewound, or deinitialized.
This is useful for parsers, request/job-local state, temporary formatting, and
batch construction.

Arena allocation calls accept optional designated arguments for allocation
behavior. Use `.zero = true` when the returned memory should be zeroed.

```c
int *values = lor_arena_alloc_array(&arena, count, sizeof(*values),
                                    .zero = true);
```

`lor_arena_mark` returns the current position inside an arena. Allocate through
the same arena as usual, then call `lor_arena_rewind` with that mark. Marks are
plain values and can be nested naturally.

```c
LorArenaMark mark = lor_arena_mark(&arena);

char *temporary = lor_arena_strdup(&arena, text);

lor_arena_rewind(&arena, mark);
```

Scratch arenas are pre-owned per-thread temporary arenas selected by
`lor_scratch_begin`. They are for short-lived helper work when the caller should
not have to create an arena. `LorScratch` is the scope handle and exposes the
selected arena for normal `lor_arena_*` allocations. Pass conflicting arenas
when nested scratch work must avoid reusing an arena whose allocations are still
live.

```c
LorScratch scratch = lor_scratch_begin(NULL, 0);
if (scratch.arena == NULL) {
    /* no scratch arena available */
    return;
}

char *text = lor_arena_strdup(scratch.arena, source);

lor_scratch_end(scratch);
```

Virtual arenas use the same designated-argument style through
`lor_arena_init`:

```c
if (!lor_arena_init(&arena, .backend = LOR_ARENA_BACKEND_VIRTUAL,
                    .reserve_size = LOR_MIB(64),
                    .commit_size = LOR_KIB(64))) {
    /* invalid config or initialization failed */
}
```

Choose `malloc` when an object has an independent lifetime or must be freed
separately. Choose an arena/temp/scratch scope when the lifetime is grouped and
bulk release makes ownership simpler.

## Cleanup Helpers

Cleanup helpers use compiler-supported scope cleanup attributes. They call the
matching explicit release function when a local variable leaves scope:

- `LOR_AUTO_FREE`: `free`
- `LOR_AUTO_ARENA`: `lor_arena_deinit`
- `LOR_AUTO_SCRATCH`: `lor_scratch_end`
- `LOR_AUTO_MMAP`: `lor_mmap_unmap`
- `LOR_AUTO_FILE`: `fclose`

They are deterministic cleanup conveniences, not leak checking. Use them for
local variables with obvious ownership; avoid them when ownership is transferred
out of the scope.

## Next Work

- Decide the first container memory policy when dynamic arrays/hash maps begin.
- Add richer leak reports only if the current report format is insufficient.
