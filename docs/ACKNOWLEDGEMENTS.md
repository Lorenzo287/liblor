# Acknowledgements

liblor is original MIT-licensed code, but its design benefited from studying
other C libraries and projects. Thank you to their authors and contributors for
publishing their work and ideas.

- [Alexey Kutepov (Tsoding)](https://github.com/tsoding) for arena, string-view,
  hash-table, dynamic-array, and `nob` experiments that demonstrate how much
  useful infrastructure can remain small and readable.
- [Sean Barrett and stb contributors](https://github.com/nothings/stb) for
  stretchy buffers, `stb_ds`, leak checking, and the broader single-header
  library approach.
- [Epic Games Tools and RAD Debugger contributors](https://github.com/EpicGamesExt/raddebugger)
  for production-oriented arena design.
- [Salvatore Sanfilippo, Oran Agra, and Redis contributors](https://github.com/antirez/sds)
  for SDS and its compact owned-string representation.
- [Daniel Holden](https://github.com/orangeduck/Cello) for Cello's exploration
  of higher-level programming techniques in C.
- [Melissa O'Neill](https://www.pcg-random.org/) for the PCG family of random
  number generators and its clear published algorithms.
- Martin Sustrik and contributors for
  [libdill](https://github.com/sustrik/libdill) and
  [libmill](https://github.com/sustrik/libmill), which informed liblor's
  structured task and channel design.
- [Viktor Kutuzov and mman-win32 contributors](https://github.com/alitrack/mman-win32)
  for documenting a portable approach to Windows memory mapping.
- [MagicalBait](https://github.com/Magicalbat) for allocator, scratch-memory,
  and random-number experiments.
- Xander Gouws for EasyArgs and its compact approach to command-line parsing.

These projects provided inspiration and comparison points. No third-party code
is copied or closely adapted into liblor's source.
