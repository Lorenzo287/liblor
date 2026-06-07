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

`lor_mmap_file` maps a complete non-empty file. `LOR_MMAP_READ` is read-only,
`LOR_MMAP_COPY` allows private changes that do not modify the file, and
`LOR_MMAP_SHARED` allows changes that are reflected in the file.

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

`LOR_LEAKCHECK` automatically enables the memory module in selective
single-header builds, so `LOR_ENABLE_STRING` plus `LOR_LEAKCHECK` is sufficient
to track dynamic-string allocations.

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
Arena leak reports use the arena's current committed backing bytes.

Leakcheck tracking uses a process-global unsynchronized list. Treat it as
single-threaded unless the caller protects all tracked memory calls externally.

## Arenas, Temps, And Scratch

Use an arena when many allocations share one lifetime. Individual arena
allocations are not freed; the whole arena is reset, rewound, or deinitialized.
This is useful for parsers, request/job-local state, temporary formatting, and
batch construction.

`lor_arena_reset` and rewinding to a zero mark clear allocation usage but keep
the arena's allocated blocks for reuse. This is a throughput-oriented high-water
policy. Use `lor_arena_deinit` when storage should be returned to the system.

Arena allocation calls are ordinary C functions. Use the `_zero` variants when
the returned memory should be zeroed.

```c
int *values = lor_arena_alloc_array_zero(&arena, count, sizeof(*values));
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
`lor_scratch_begin`. The library automatically provisions 2 of these arenas per
thread with zero setup required, they are ready to use just by including the header.
They are perfect for short-lived helper work when the caller should not have
to create a dedicated arena.

`LorScratch` is the scope handle and exposes the selected arena for normal
`lor_arena_*` allocations. 

Pass conflicting arenas when nested scratch work must avoid reusing an arena
whose allocations are still live. Since the library provides 2 scratch arenas,
passing an output arena as a conflict guarantees the library will hand you
the *other* scratch arena, allowing you to safely build messy temporary
structures without overwriting the clean data you intend to return.

```c
LorScratch scratch = lor_scratch_begin(NULL, 0);
if (scratch.arena == NULL) {
    /* no scratch arena available */
    return;
}

char *text = lor_arena_strdup(scratch.arena, source);

lor_scratch_end(scratch);
```

Virtual arenas use explicit configuration through `lor_arena_init_config`:

```c
if (!lor_arena_init_config(&arena,
                           (LorArenaConfig){
                               .backend = LOR_ARENA_BACKEND_VIRTUAL,
                               .reserve_size = LOR_MIB(64),
                               .commit_size = LOR_KIB(64),
                           })) {
    /* invalid config or initialization failed */
}
```

Arena accounting excludes block metadata and leading alignment slack:

- `lor_arena_used` reports consumed usable capacity, including alignment
  padding between allocations.
- `lor_arena_capacity` reports all usable capacity currently held by the arena.
- `lor_arena_committed` reports the usable portion currently backed by
  committed memory.

For heap arenas, each block is fully backed when allocated, so capacity and
committed memory are equal. For virtual arenas, capacity is reserved address
space and can be much larger than committed memory. Virtual memory is committed
on demand in commit-size increments. Resetting an arena, or rewinding within a
retained block, lowers used bytes but does not decommit storage. Rewinding can
still lower capacity and committed memory when it releases blocks created after
the mark.

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
- `LOR_AUTO_STRING`: `lor_string_deinit` (declared by `lor/string.h`)
- `LOR_AUTO_ARRAY`: `lor_array_deinit` (declared by `lor/array.h`)
- `LOR_AUTO_MAP`: `lor_map_deinit` (declared by `lor/map.h`)
- `LOR_AUTO_SET`: `lor_set_deinit` (declared by `lor/set.h`)

They are deterministic cleanup conveniences, not leak checking. Use them for
local variables with obvious ownership; avoid them when ownership is transferred
out of the scope.

## Next Work

- Add richer leak reports only if the current report format is insufficient.
