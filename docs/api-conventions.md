# API Conventions

These conventions keep liblor modules consistent without requiring a generic
allocator, exception system, or result macro before real use cases justify
them.

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

There is no public `LorAllocator`. Add one only when at least two concrete
modules need configurable allocation and their requirements are understood.

## Failure

- Liblor does not print, terminate, or modify global error state for recoverable
  failures.
- Predicates and searches use integer results, with output parameters when
  additional values are needed. A normal "not found" result is not an error.
- Operations with multiple meaningful failure reasons return `LorStatus`.
- Module-specific result structures may be added when callers need context that
  a status alone cannot carry.
- Allocating mutators leave the original owned object unchanged on failure.
- Initializing constructors leave their output safely deinitializable on
  failure.
- Arithmetic is checked before allocation or pointer-size calculations.
- Assertions are reserved for internal invariants and documented programmer
  errors, not allocation, input, or platform failures.

The initial shared statuses are intentionally small:

- `LOR_STATUS_OK`
- `LOR_STATUS_INVALID_ARGUMENT`
- `LOR_STATUS_OUT_OF_MEMORY`
- `LOR_STATUS_OVERFLOW`

Add statuses only when a public operation needs callers to distinguish a new
failure category.

## State And Threading

- `deinit` functions accept `NULL` and reset valid objects to their initializer
  state.
- Unless documented otherwise, separate objects may be used concurrently but
  the same mutable object requires external synchronization.
- Process-global or thread-local state must be explicit in the module design.
