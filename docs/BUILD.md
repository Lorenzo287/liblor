# Building liblor

The repository Makefile builds the multi-file library, generated single header,
tests, and examples. Generated output is stored under `.build/`.

## Development

- `make all`: build and run tests, build examples, and regenerate `lor.h`.
- `make test`: build and run tests.
- `make example`: build multi-file examples.
- `make example-sh`: build single-header examples.
- `make clang`: run the normal Clang profile in `.build/clang/`.
- `make gcc`: run the normal GCC profile in `.build/gcc/`.
- `make strict`: run Clang with warnings treated as errors.
- `make leakcheck`: run the strict Clang profile with `LOR_LEAKCHECK`.
- `make check`: run strict Clang, strict GCC, and leak-check profiles.
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
- `make release-lto`: build a compiler-specific release with link-time
  optimization.
- `make release-lto-check`: test the LTO static and shared consumers.

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
