# liblor: Agent Manual

liblor is a higher-level C library. Version 1 is feature complete.

## Project Map

- `README.md`: public project overview.
- `docs/ROADMAP.md`: current work and next decisions.
- `docs/ACKNOWLEDGEMENTS.md`: projects and authors that inspired liblor.
- `docs/BUILD.md`: detailed local build, test, and release instructions.
- `Makefile`: local development build/test entry point.
- `compile_flags.txt`: clangd include/diagnostic flags.
- `docs/memory.md`: memory subsystem implementation path.
- `tools/gen_single_header.py`: generates `lor.h`.
- `tools/lor_modules.json`: single-header module and alias manifest.
- `lor.h`: generated single-header liblor; do not edit by hand.
- `include/lor/`, `src/`, `tests/`, `examples/`: liblor-owned source.

## Workflow

- Shell: assume Windows PowerShell.
- Start with `git status --short`; do not overwrite user changes.
- Read `docs/ROADMAP.md` before choosing the next task.
- Read `docs/memory.md` before allocator, arena, cleanup, leak-checking,
  refcount, or GC work.
- Search with `rg` or `rg --files` when available.
- Build with `make all`; use `make gcc` for GCC and `make check` for the full
  strict Clang, strict GCC, and leak-check verification.
- Build optimized static and shared libraries with `make release-check`.
- Regenerate the single header with `make single-header` after public API or
  implementation changes.
- The Makefile discovers `src/*.c`, `tests/test_*.c`, `examples/*.c`, and
  `include/lor/*.h`; update it only for build behavior changes.
- Update `tools/lor_modules.json` when generated `lor.h` needs a new public
  module, enable macro, or prefix alias.
- Record any future external design influence or adapted code in
  `docs/ACKNOWLEDGEMENTS.md`.

## C Style

- A local `.clang-format` may be used as a formatting reference, but it is not a
  tracked project requirement.
- Prefer C99/C11-compatible code unless a module documents otherwise.
- Use `snake_case` for functions, variables, and fields.
- Declare variables close to first use and initialize them in the declaration
  when practical. Prefer loop-local declarations over function-wide counters.
- File-local helpers should be `static`.
- File-local helpers in module sources should use a module-qualified internal
  name such as `lor_arena__block_new`, because source files are amalgamated
  into generated `lor.h`.
- Omit braces for simple single-line `if`, `while`, `for`, and similar
  statements. Keep braces when a body has multiple statements or they make
  nested control flow clearer.
- Use `//` for short comments. Use `/* ... */` for multi-line comments without
  adding a leading `*` to every continuation line, and close the comment at the
  end of its final text line.
- Comments should explain intent, invariants, ownership, or portability issues.

## API Rules

- Ownership must be obvious from names or docs.
- Allocation/freeing rules must be consistent across modules.
- Failure behavior must be explicit before implementation.
- Avoid global mutable state unless the module is explicitly designed around it.
- Keep platform-specific behavior behind small boundaries.

## Licensing Rules

- liblor-owned code is MIT-licensed.
- New liblor-owned source files may use `// SPDX-License-Identifier: MIT`.
- Do not remove upstream copyright notices.
- Copied or closely adapted code must preserve required license text.
- Inspiration-only rewrites may stay MIT, but meaningful influence should be
  credited in `docs/ACKNOWLEDGEMENTS.md`.
