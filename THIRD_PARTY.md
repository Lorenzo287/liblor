# Third Party And Inspirations

liblor-owned source code is licensed under the MIT License. This file tracks
external code and design influences so license obligations and credits stay
visible as the project grows.

Status labels:

- `reference`: present in the repository as study material, not part of liblor's
  public implementation.
- `inspiration`: influenced liblor design, but no code was copied.
- `adapted`: liblor contains code closely derived from the source.
- `copied`: liblor contains source code copied from the project.
- `needs audit`: do not use until license/source details are clear.

## Current liblor Source

No third-party code has been copied or closely adapted into the liblor-owned
source tree yet.

## Reference And Inspiration Inventory

| Path | Source / Author | License | Status | Notes |
| --- | --- | --- | --- | --- |
| `references/arena/tsoding/` | Alexey Kutepov / Tsoding | MIT | inspiration | Arena allocator reference. Current `src/arena.c` is liblor-owned code, not copied. |
| `references/arena/magicalbait/` | MagicalBait | needs audit | needs audit | Interesting arena and PRNG snippets. `prng.h` mentions Apache-2.0; other files need source/license confirmation before use. |
| `references/build/nob/` | Alexey Kutepov / Tsoding | MIT or public domain | inspiration | C-native build helper reference. Current `dev.c` is liblor-owned code, not copied. |
| `references/build/nob_modified/` | Alexey Kutepov / Tsoding, modified locally | MIT or public domain | reference | Audit local changes before use. |
| `references/strings/sds/` | Salvatore Sanfilippo, Oran Agra, Redis contributors | BSD-style | reference | Dynamic string reference. Preserve notices if adapted. |
| `references/strings/string_view/` | Alexey Kutepov / Tsoding | MIT | reference | String-view reference. |
| `references/dynamic_array/stb_ds.h` | Sean Barrett / stb | MIT or public domain | reference | Dynamic array/hash map reference. |
| `references/dynamic_array/stretchy_buffer.h` | Sean Barrett / stb | MIT or public domain | reference | Stretchy-buffer reference. |
| `references/hash_table/stb/` | Sean Barrett / stb | MIT or public domain | reference | Hash-table reference. |
| `references/hash_table/tsoding/` | Alexey Kutepov / Tsoding | MIT or public domain | reference | Hash-table reference. |
| `references/cleanup (auto free)/stb_leakcheck.h` | Sean Barrett / stb | MIT or public domain | reference | Leak-checking reference. |
| `references/leakcheck/stb_leakcheck.h` | Sean Barrett / stb | MIT or public domain | reference | Leak-checking reference. |
| `references/libCello/` | Daniel Holden | BSD-style | inspiration | Higher-level C design reference. |
| `references/mman (mmap)/` | Viktor Kutuzov, Klaus Post, Hermann Seib | MIT | reference | Windows mmap compatibility reference. |
| `references/rand/` | Lorenzo Tumini; PCG inspiration by Melissa O'Neill | Apache-2.0 | reference | Treat carefully if reused; Apache-2.0 obligations apply to copied/adapted code. |
| `references/concurrency/libdill/` | Martin Sustrik and contributors | MIT-like permissive | reference | Coroutine/concurrency reference. Preserve notices if adapted. |
| `references/concurrency/libmill/` | Martin Sustrik | MIT-like permissive | reference | Coroutine/concurrency reference. Audit before use. |
| `references/stb_misc/stb.h` | Sean Barrett / stb | MIT or public domain | reference | Large utility header. Read only for specific needs. |

Smaller experiment folders should be audited and added here before their ideas
or code move into liblor.
