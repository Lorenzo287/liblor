# liblor: Agent Manual

liblor is a personal higher-level C library. It is currently an inspiration
workbench: most top-level folders are references, snippets, or experiments, not
the final library source.

Roadmap work lives in `ROADMAP.md`. Keep this file focused on navigation,
development rules, and agent expectations.

## Project Map

- `README.md`: public-facing project overview.
- `ROADMAP.md`: current plan, priorities, and progress tracking.
- `THIRD_PARTY.md`: license and attribution tracking.
- `dev.c`: tiny development helper for building/testing liblor itself.
- `docs/build-system.md`: direction for the future liblor build/run/REPL tool.
- `include/lor/`: public liblor headers.
- `src/`: liblor implementations.
- `tests/`: focused module tests.
- `examples/`: small runnable examples.
- `references/`: third-party libraries, snippets, and experiments used only for
  study and audit until explicitly moved into liblor-owned source.

Current liblor-owned source layout:

- `include/lor/`: public headers.
- `src/`: implementation files.
- `tests/`: module tests.
- `examples/`: small usage examples.
- `docs/`: design notes.
- Future `tools/`: project-local tooling.

## Fast Context

- Start every coding session by reading `ROADMAP.md`.
- Treat existing idea folders as source material, not final architecture.
- Prefer small, cohesive modules over broad utility dumping.
- Do not import entire external libraries unless the roadmap explicitly says to.
- Before reading a large dependency, inspect its README, license, and relevant
  small files first.
- The current folder is not necessarily a Git repository. Try
  `git status --short`; if it fails, continue without Git assumptions.
- Development helper bootstrap: `clang -std=c11 -Wall -Wextra -Wpedantic dev.c
  -o liblor-dev.exe`.
- Run tests: `.\liblor-dev.exe all`.
- Use `$env:CC = 'gcc'` before running the helper to compile liblor with GCC.

## Workflow

- Shell: assume Windows PowerShell.
- Use PowerShell syntax in commands and documentation examples.
- Use `rg` or `rg --files` for searching when available.
- Preserve user changes. Never revert files unless the user explicitly asks.
- Before changing code, inspect nearby files and match local patterns.
- Before adding a new liblor module, define its public API, ownership rules, and
  error behavior.
- For each imported or rewritten idea, record its source and license status in
  `THIRD_PARTY.md`.
- Keep README public-facing, AGENTS operational, and ROADMAP tactical.

## C Style

- Prefer C99/C11-compatible code unless a module has a documented reason to
  require something newer.
- Use 4-space indentation.
- Use `snake_case` for functions, variables, and fields.
- Public functions use `lor_`.
- Public types use `LorName`.
- Public constants and feature macros use `LOR_NAME`.
- Internal project-wide helpers may use `lor_` plus an internal name until a
  better convention is chosen.
- File-local helpers should be `static`.
- Keep macros simple and documented. Use macros only when they provide real C
  ergonomics that functions cannot provide.
- Prefer clear loops and direct control flow over dense macro cleverness.
- Comments should explain intent, invariants, ownership, or portability issues.
  Do not add comments that merely restate the line of code.

## API Rules

- Ownership must be obvious from the function name or documentation.
- Allocation and freeing must be paired consistently across modules.
- Prefer explicit allocator parameters or documented default allocators for data
  structures.
- Functions that can fail should have a clear failure convention before they are
  implemented.
- Avoid global mutable state unless a module is explicitly designed around it.
- Keep platform-specific behavior isolated behind small internal boundaries.
- Design APIs for composition: strings, arrays, maps, errors, and allocators
  should be able to work together without special cases.

## Licensing Rules

- liblor-owned code is MIT-licensed.
- New liblor-owned source files may use `// SPDX-License-Identifier: MIT`
  instead of copying the full MIT license block.
- Do not copy code from inspiration folders until its license has been checked.
- Do not remove upstream copyright notices.
- If code is copied or closely adapted, preserve required license text and note
  the source.
- If only an idea is used, still record design credit when the influence is
  meaningful.
- Be cautious with Apache-2.0 sources: copied or adapted Apache-2.0 code keeps
  Apache-2.0 notice obligations even when liblor's own code is MIT.
- Add or update an attribution file before the first real release.

## Documentation Rules

- Public docs should avoid promising stable APIs before they exist.
- Roadmap entries should show status and the next concrete action.
- Prefer short examples once APIs exist.
- Keep Windows build instructions accurate; do not publish bash-only workflows
  unless cross-platform instructions are intentionally added.
