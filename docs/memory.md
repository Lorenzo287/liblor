# Memory

The memory module provides arenas, scratch scopes, file mapping, and automatic
cleanup helpers.

## Features

- `LorArena`: heap-backed by default, with optional virtual-memory backend.
- `LorArenaMark` / `lor_arena_mark` / `lor_arena_rewind`: explicit
  checkpoints for rewinding temporary allocations inside the same arena.
- `LorScratch` / `lor_scratch_begin`: scope handles for per-thread scratch arenas.
- `LorMmap`: file mapping by path.
- cleanup helpers: scope-exit cleanup for heap pointers, arenas, scratch scopes,
  mmap mappings, and `FILE *` handles on compilers that support cleanup
  attributes (they are also available for other liblor modules).
- `LOR_LEAKCHECK`: development build mode for location-aware leak checking.

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
LorArena arena = LOR_ARENA_INIT;
int *values = lor_arena_alloc_array_zero(&arena, count, sizeof(*values));
```

Default heap arenas require no explicit initialization. Their configuration is
established lazily by the first allocation. Always begin an arena in the
`LOR_ARENA_INIT` state and eventually call `lor_arena_deinit`.

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

Nested scratch work does not inherently require a conflict. If an inner scope
reuses the outer scope's arena, its mark is taken after the outer allocations,
so ending the inner scope preserves those earlier allocations and discards only
the inner ones.

A conflict is needed when a helper uses scratch memory for temporary work but
also allocates a result into a caller-provided scratch arena that must remain
valid after the helper returns. If the helper selected that same arena for its
own scratch scope, the result would be allocated after the helper's mark and
`lor_scratch_end` would rewind it along with the temporary allocations. Pass
the result arena as the conflict so the helper receives the other thread-local
scratch arena:

```c
char *copy_result(LorArena *result_arena, const char *source) {
    LorScratch scratch = lor_scratch_begin(result_arena);
    if (scratch.arena == NULL) return NULL;

    char *temporary = lor_arena_strdup(scratch.arena, source);
    char *result =
        temporary != NULL ? lor_arena_strdup(result_arena, temporary) : NULL;

    lor_scratch_end(scratch);
    return result;
}
```

If every allocation made after the inner scope begins is temporary and may be
discarded when that scope ends, pass `NULL`; reusing the outer scratch arena is
safe in that case.

The general scratch pattern remains:

```c
LorScratch scratch = lor_scratch_begin(NULL);
if (scratch.arena == NULL) {
    /* no scratch arena available */
    return;
}

char *text = lor_arena_strdup(scratch.arena, source);

lor_scratch_end(scratch);
```

Virtual arenas use explicit configuration through `lor_arena_init_config`:

```c
LorArena arena = LOR_ARENA_INIT;
if (!lor_arena_init_config(&arena,
                           (LorArenaConfig){
                               .backend = LOR_ARENA_BACKEND_VIRTUAL,
                               .reserve_size = LOR_MIB(64),
                               .commit_size = LOR_KIB(64),
                           })) {
    /* invalid config or initialization failed */
}
```

`lor_arena_init_config` is only for a new arena in the `LOR_ARENA_INIT` state;
it does not reconfigure an active arena. Deinitialize an arena before
initializing it again with a different configuration.

`lor_page_size()` returns the host operating system page size or a conservative
fallback. It can be used to config the backend to use multiples of the OS pages
for reserve and commit operations.

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

## File Mapping

`lor_mmap_file` maps a complete non-empty file.
`LOR_MMAP_READ` is read-only, `LOR_MMAP_COPY` allows private changes that
do not modify the file, and `LOR_MMAP_SHARED` allows changes that are reflected in the file.

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
make leakcheck
```

For multi-file builds, compile `src/memory.c` with `LOR_LEAKCHECK` to enable
tracking and define it in consuming translation units to capture call-site file
and line. In practice, pass `-DLOR_LEAKCHECK` to the whole project build.

When active, leakcheck tracks liblor heap helpers, active arenas, virtual
arena lifetimes, and active mmap mappings. It does not own or clean up
resources; it only reports resources whose matching release/deinit/free/unmap
call was not made.
Arena leak reports use the arena's current committed backing bytes.

Leakcheck query and reporting calls may be left in program code unconditionally:

```c
printf("tracked resources: %zu\n", lor_leakcheck_count());
(void)lor_leakcheck_report(stderr);
```

In builds without `LOR_LEAKCHECK`, these calls are silent: counts and statistics
are zero, and reports write nothing. This allows development instrumentation to
remain in source without changing release-build output.

`lor_leakcheck_is_enabled()` returns non-zero when the linked memory
implementation was compiled with `LOR_LEAKCHECK`. Use it when a program needs
to display the current mode or reject a run that requires tracking. Unlike a
preprocessor check in an application source file, it reports the mode compiled
into `src/memory.c`, which helps expose mismatched flags in multi-file builds.

`lor_leakcheck_stats()` returns a `LorLeakStats` structure with current
allocation counts and byte totals. `lor_leakcheck_count()` returns the number
of active tracking records.

Leakcheck tracking uses a process-global unsynchronized list. Treat it as
single-threaded unless the caller protects all tracked memory calls externally.

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
- `LOR_AUTO_CLI`: `lor_cli_deinit` (declared by `lor/cli.h`)

They are deterministic cleanup conveniences, not leak checking. Use them for
local variables with obvious ownership; avoid them when ownership is transferred
out of the scope.
