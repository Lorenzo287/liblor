# Acknowledgements

liblor is original MIT-licensed code, but its design benefited from studying
other C libraries and projects. Thank you to their authors and contributors for
publishing their work and ideas.

- [Alexey Kutepov (Tsoding)](https://github.com/tsoding) for arena, string-view,
  hash-table, dynamic-array, and `nob` experiments that demonstrate how much
  useful infrastructure can remain small and readable. His `sv` library informed
  liblor's string views; his `ht.h` informed the typed generic API and custom 
  hash/equality direction of liblor's hash maps; and his `nob` dynamic-array
  utilities informed the explicit pointer-to-handle mutation and typed macro
  surface of liblor's dynamic arrays.
- [Sean Barrett and stb contributors](https://github.com/nothings/stb) for
  stretchy buffers, `stb_ds`, leak checking, and the broader single-header library
  approach. Sean Barrett's stretchy buffer and `stb_ds` dynamic arrays informed
  liblor's dynamic array design, and `stb_ds` also informed the dense entries,
  open addressing, and string-key policy of liblor's hash maps.
- [Epic Games Tools and RAD Debugger contributors](https://github.com/EpicGamesExt/raddebugger)
  for production-oriented arena design, along with the arena and scratch-space
  patterns used in the Ryan Fleury and Mr. 4th Programming communities, which together
  shaped liblor's arena implementation.
- [Salvatore Sanfilippo, Oran Agra, and Redis contributors](https://github.com/antirez/sds)
  for SDS and its compact owned-string representation — particularly its explicit length,
  spare capacity, binary safety, NUL compatibility, and geometric growth — which
  directly informed liblor's owned string design.
- [Daniel Holden](https://github.com/orangeduck/Cello) for Cello's exploration
  of higher-level programming techniques in C.
- [Melissa O'Neill](https://www.pcg-random.org/) for the PCG family of random
  number generators and its clear published algorithms. Liblor's pseudorandom
  generation uses her PCG32 XSH-RR algorithm.
- Martin Sustrik and contributors for [libdill](https://github.com/sustrik/libdill)
  and [libmill](https://github.com/sustrik/libmill), which informed liblor's
  structured task and channel design.
- [Viktor Kutuzov and mman-win32 contributors](https://github.com/alitrack/mman-win32)
  for documenting a portable approach to Windows memory mapping.
- [MagicalBait](https://github.com/Magicalbat) for allocator, scratch-memory,
  and random-number experiments.
- Xander Gouws for EasyArgs and its compact approach to command-line parsing,
  which alongside Python's `argparse` and Go's `flag` package inspired liblor's
  metadata-driven CLI parsing interface.
