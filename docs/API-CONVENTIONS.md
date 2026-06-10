# API Conventions

These conventions keep liblor modules consistent.

## Ownership And Lifetime

- Borrowed values use view types such as `LorStringView`. They never free or
  extend the referenced storage.
- Owned values use explicit `init` / `deinit` lifetimes and document whether
  zero initialization is valid.
- Independently owned objects use the C heap unless an API explicitly accepts
  an arena.
- Ownership transfer must be explicit in the function name or documentation.
- Pointers into growable owned objects become invalid when an operation may
  reallocate them.

Generic typed containers may use function-like macros to infer information
that C cannot pass generically, such as `sizeof *(array)`. Compiler-extension
convenience macros must have a feature macro and a documented portable
alternative. Keep allocation, overflow checking, and ownership changes in
ordinary implementation functions rather than duplicating them in macros.
Shared extension checks and declaration inference live in `lor/features.h`.

Generic value dispatch may use standard C11 `_Generic`. Unsupported values
must fail at compile time or require an explicit wrapper; they must not silently
fall through to an incompatible variadic format. Generic printing follows this
rule and keeps I/O behavior in an ordinary tagged-value function. Pointer-handle
containers require family-specific wrappers because C's type system cannot
distinguish an `int *` array from an `int *` set or an ordinary `int *`.

## Failure

- Liblor does not print, terminate, or modify global error state for recoverable
  failures.
- Predicates and searches use integer results, with output parameters when
  additional values are needed. A normal "not found" result is not an error.
- Operations with multiple meaningful failure reasons return `LorStatus`.
- Module-specific result structures may be added when callers need context that
  a status alone cannot carry. `LorCliResult` follows this rule for parse
  errors that include an argument index, token, and option or positional name.
- Allocating mutators leave the original owned object unchanged on failure.
- Initializing constructors leave their output safely deinitializable on
  failure.
- Arithmetic is checked before allocation or pointer-size calculations.
- Assertions are reserved for internal invariants and documented programmer
  errors, not allocation, input, or platform failures.

The shared statuses include:

- `LOR_STATUS_OK`
- `LOR_STATUS_INVALID_ARGUMENT`
- `LOR_STATUS_OUT_OF_MEMORY`
- `LOR_STATUS_OVERFLOW`
- `LOR_STATUS_SYSTEM_ERROR`
- `LOR_STATUS_TIMED_OUT`
- `LOR_STATUS_CLOSED`

`lor_status_name(status)` returns a stable lowercase name for a status code.

## State And Threading

- `deinit` functions accept `NULL` and reset valid objects to their initializer
  state.
- Unless documented otherwise, separate objects may be used concurrently but
  the same mutable object requires external synchronization.
- Process-global or thread-local state must be explicit in the module design.
