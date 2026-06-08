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

| Source / Path                     | Author                                              | License              | Status      | Notes                                                                                        |
| --------------------------------- | --------------------------------------------------- | -------------------- | ----------- | -------------------------------------------------------------------------------------------- |
| Tsoding arena allocator           | Alexey Kutepov / Tsoding                            | MIT                  | inspiration | Arena study copy removed. Current `src/memory.c` is liblor-owned code, not copied.           |
| MagicalBait arena snippets        | MagicalBait                                         | MIT                  | inspiration | Arena and scratch design study copy removed.                                                 |
| RAD Debugger arena                | Epic Games Tools / RAD Debugger                     | MIT                  | inspiration | Production arena study copy removed. Current memory code is original.                        |
| `references/build/nob/`           | Alexey Kutepov / Tsoding                            | MIT or public domain | inspiration | Informed build tooling and dynamic-array ergonomics; liblor code is original.                |
| SDS                               | Salvatore Sanfilippo, Oran Agra, Redis contributors | BSD-style            | inspiration | String study copy removed. Influenced prefix-header strings; liblor code is original.        |
| Tsoding string view               | Alexey Kutepov / Tsoding                            | MIT                  | inspiration | String-view study copy removed. Influenced parsing operations; liblor code is original.      |
| stb_ds dynamic arrays             | Sean Barrett / stb                                  | MIT or public domain | inspiration | Array study copy removed. Informed typed-pointer arrays; liblor code is original.            |
| stb stretchy buffer               | Sean Barrett / stb                                  | MIT or public domain | inspiration | Array study copy removed. Informed prefix headers and indexing; liblor code is original.     |
| stb_ds hash table                 | Sean Barrett / stb                                  | MIT or public domain | inspiration | Removed. Informed dense hash-map entries and string-key policy; liblor code is original.     |
| Tsoding hash table                | Alexey Kutepov / Tsoding                            | MIT or public domain | inspiration | Removed. Informed typed generic hash/equality APIs; liblor code is original.                 |
| stb leakcheck                     | Sean Barrett / stb                                  | MIT or public domain | inspiration | Leak-checking study copy removed; liblor implementation is original.                         |
| libCello                          | Daniel Holden                                       | BSD-style            | inspiration | Removed. Its test framework motivated the shared test assertion header; no code was adapted. |
| mman-win32                        | Viktor Kutuzov, Klaus Post, Hermann Seib            | MIT                  | inspiration | Windows mmap study copy removed; liblor uses a narrower native abstraction.                  |
| PCG, random                       | Melissa O'Neill; Lorenzo Tumini; MagicalBait        | Apache-2.0 / MIT     | inspiration | Removed. Informed by PCG32 XSH-RR; liblor implements the published algorithm independently.  |
| print and type utils              | Lorenzo Tumini                                      | project-owned / MIT  | inspiration | Removed. Production code was rewritten around tagged values and explicit failure behavior.   |
| libdill                           | Martin Sustrik and contributors                     | MIT-like permissive  | inspiration | Removed. Informed structured task ownership and cooperative cancellation; liblor code is original. |
| libmill                           | Martin Sustrik                                      | MIT-like permissive  | inspiration | Removed. Informed typed value-copy channels and deadline-based waits; liblor code is original. |
| Sean's Tool Box (`stb.h`)         | Sean Barrett / stb                                  | MIT or public domain | inspiration | Removed. Its numeric conveniences motivated original single-evaluation helpers.              |
| EasyArgs cli parser               | Xander Gouws                                        | MIT                  | inspiration | Removed. EasyArgs informed basic CLI scope; liblor uses an original function API.            |

Smaller experiment folders should be audited and added here before their ideas
or code move into liblor.
