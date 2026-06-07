# Third Party And Inspirations

liblor-owned source code is licensed under the MIT License. This file tracks
external code and design influences so license obligations and credits stay
visible as the project grows.

Status labels:

- `reference`: present in the repository as study material, not part of liblor's
  public implementation.
- `inspiration`: influenced liblor design, but no code was copied. The study
  copy may have been removed after the design was settled.
- `adapted`: liblor contains code closely derived from the source.
- `copied`: liblor contains source code copied from the project.
- `needs audit`: do not use until license/source details are clear.

## Current liblor Source

No third-party code has been copied or closely adapted into the liblor-owned
source tree yet.

## Reference And Inspiration Inventory

| Source / Path                                | Author                                              | License              | Status      | Notes                                                                              |
| -------------------------------------------- | --------------------------------------------------- | -------------------- | ----------- | ---------------------------------------------------------------------------------- |
| Tsoding arena allocator                      | Alexey Kutepov / Tsoding                            | MIT                  | inspiration | Arena study copy removed. Current `src/memory.c` is liblor-owned code, not copied. |
| MagicalBait arena snippets                   | MagicalBait                                         | MIT                  | inspiration | Arena and scratch design study copy removed.                                       |
| RAD Debugger arena                           | Epic Games Tools / RAD Debugger                     | MIT                  | inspiration | Production arena study copy removed. Current memory code is original.              |
| `references/build/nob/`                      | Alexey Kutepov / Tsoding                            | MIT or public domain | inspiration | Informed build tooling and dynamic-array ergonomics; liblor code is original.      |
| `references/strings/sds/`                    | Salvatore Sanfilippo, Oran Agra, Redis contributors | BSD-style            | inspiration | Influenced dynamic strings and prefix headers; liblor code is original.            |
| `references/strings/string_view/`            | Alexey Kutepov / Tsoding                            | MIT                  | inspiration | Influenced string-view operations; liblor code is original.                        |
| `references/dynamic_array/stb_ds.h`          | Sean Barrett / stb                                  | MIT or public domain | inspiration | Informed typed-pointer arrays and prefix headers; liblor code is original.         |
| `references/dynamic_array/stretchy_buffer.h` | Sean Barrett / stb                                  | MIT or public domain | inspiration | Informed typed-pointer arrays and direct indexing; liblor code is original.        |
| `references/hash_table/stb/`                 | Sean Barrett / stb                                  | MIT or public domain | reference   | Hash-table reference.                                                              |
| `references/hash_table/tsoding/`             | Alexey Kutepov / Tsoding                            | MIT or public domain | reference   | Hash-table reference.                                                              |
| stb leakcheck                                | Sean Barrett / stb                                  | MIT or public domain | inspiration | Leak-checking study copy removed; liblor implementation is original.               |
| `references/libCello/`                       | Daniel Holden                                       | BSD-style            | inspiration | Higher-level C design reference.                                                   |
| mman-win32                                   | Viktor Kutuzov, Klaus Post, Hermann Seib            | MIT                  | inspiration | Windows mmap study copy removed; liblor uses a narrower native abstraction.        |
| `references/rand/`                           | MagicalBait; PCG inspiration by Melissa O'Neill     | Apache-2.0           | reference   | Treat carefully if reused; Apache-2.0 obligations apply to copied/adapted code.    |
| `references/concurrency/libdill/`            | Martin Sustrik and contributors                     | MIT-like permissive  | reference   | Coroutine/concurrency reference. Preserve notices if adapted.                      |
| `references/concurrency/libmill/`            | Martin Sustrik                                      | MIT-like permissive  | reference   | Coroutine/concurrency reference. Audit before use.                                 |
| `references/stb_misc/stb.h`                  | Sean Barrett / stb                                  | MIT or public domain | reference   | Large utility header. Read only for specific needs.                                |

Smaller experiment folders should be audited and added here before their ideas
or code move into liblor.
