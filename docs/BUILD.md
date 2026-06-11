# Building liblor

The repository Makefile builds the multi-file library, generated single header,
tests, and examples. Generated output is stored under `.build/`.

## Development

- `make` or `make all`: compile the multi-file library objects and regenerate
  `lor.h` when its inputs changed.
- `make test`: build and run tests.
- `make examples`: build multi-file and single-header examples.
- `make gcc`: compile the normal GCC profile in `.build/gcc/`.
- `make leakcheck`: run tests with strict Clang and `LOR_LEAKCHECK`.
- `make check`: run tests and build examples with strict Clang and strict GCC,
  then run the leak-check tests.
- `make clean`: remove `.build/`.

The Makefile discovers `src/*.c`, `tests/test_*.c`, `examples/*.c`,
`examples_sh/*.c`, and `include/lor/*.h` automatically.

## Single Header

Run:

```powershell
make single-header
```

This regenerates the repository-root `lor.h` from the public headers, module
sources, and `tools/lor_modules.json`. Do not edit `lor.h` directly.

## Release Libraries

- `make release`: build optimized static and shared libraries.
- `make release-check`: build the release libraries and test static and shared
  consumers.

Set `RELEASE_LTO` when link-time optimization is wanted. For example:

```powershell
make RELEASE_LTO=-flto release-check
```

Release builds use `-O3 -DNDEBUG` without CPU-specific flags. Normal artifacts
are written under `.build/release/<compiler>/`; LTO profiles add `-lto` to the
compiler directory name.

Expected library files:

- Windows with Clang: `lor_static.lib`, `lor.dll`, and `lor.lib`.
- Windows with GCC: `liblor.a`, `lor.dll`, and `liblor.dll.a`.
- Linux: `liblor.a` and `liblor.so`.
- macOS: `liblor.a` and `liblor.dylib`.

Compiled libraries contain every multi-file module implementation. Consumers
include headers from `include/lor/`; the generated `lor.h` is only for the
single-header distribution.

On Windows, DLL exports are generated from `tools/lor_modules.json`, which is
the public-symbol source of truth for both the release libraries and generated
single header.
