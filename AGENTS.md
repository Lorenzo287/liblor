# liblor: Agent Manual

liblor is an early higher-level C library. Most code under `references/` is
study material, not liblor-owned source.

## Project Map

- `README.md`: public project overview.
- `docs/ROADMAP.md`: current work and next decisions.
- `docs/THIRD_PARTY.md`: attribution and license audit tracking.
- `Makefile`: local development build/test entry point.
- `compile_flags.txt`: clangd include/diagnostic flags.
- `docs/build-system.md`: future `lor run` / `lor build` / REPL direction.
- `docs/memory.md`: memory subsystem implementation path.
- `tools/gen_single_header.py`: generates `lor.h`.
- `tools/lor_modules.json`: single-header module and alias manifest.
- `lor.h`: generated single-header liblor; do not edit by hand.
- `include/lor/`, `src/`, `tests/`, `examples/`: liblor-owned source.
- `references/`: third-party libraries, snippets, and experiments.

## Workflow

- Shell: assume Windows PowerShell.
- Start with `git status --short`; do not overwrite user changes.
- Read `docs/ROADMAP.md` before choosing the next task.
- Read `docs/memory.md` before allocator, arena, cleanup, leak-checking,
  refcount, or GC work.
- Search with `rg` or `rg --files` when available.
- Build with `make all`; use `make CC=gcc all` for GCC.
- Regenerate the single header with `make single-header` after public API or
  implementation changes.
- The Makefile discovers `src/*.c`, `tests/test_*.c`, `examples/*.c`, and
  `include/lor/*.h`; update it only for build behavior changes.
- Update `tools/lor_modules.json` when generated `lor.h` needs a new public
  module, enable macro, or prefix alias.
- Before using anything from `references/`, inspect its README/license and
  record the decision in `docs/THIRD_PARTY.md`.

## C Style

- A local `.clang-format` may be used as a formatting reference, but it is not a
  tracked project requirement.
- Prefer C99/C11-compatible code unless a module documents otherwise.
- Use `snake_case` for functions, variables, and fields.
- File-local helpers should be `static`.
- File-local helpers in module sources should use a module-qualified internal
  name such as `lor_arena__block_new`, because source files are amalgamated
  into generated `lor.h`.
- Braces may be omitted for simple single-line `if`, `while`, and similar
  statements.
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
  credited in `docs/THIRD_PARTY.md`.
