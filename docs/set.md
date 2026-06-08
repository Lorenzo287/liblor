# Sets

liblor sets are a thin key-only facade over the hash-map implementation. A set
handle is a typed pointer directly to its densely stored keys:

```c
int *numbers = LOR_SET_INIT;

lor_set_add_as(numbers, int, 10);
lor_set_add_as(numbers, int, 20);
lor_set_add_as(numbers, int, 10); // still two keys

for (size_t i = 0; i < lor_set_size(numbers); ++i)
    printf("%d\n", numbers[i]);

lor_set_deinit(&numbers);
```

Sets reuse map hashing, equality, collision handling, allocation, key
ownership, leak checking, and cleanup. There is no second hash-table
implementation.

Iteration order is unspecified and may change after insertion or removal.
Pointers to keys may be invalidated by insertion, reserve, or removal. Stored
keys must not be modified directly because their hash-table positions depend
on their contents.

## Key Policies

A zero-initialized set uses byte hashing and equality, which is appropriate for
integer, enum, and pointer-identity keys:

```c
unsigned *ids = LOR_SET_INIT;
```

Custom hash/equality, validation, cloning, and dropping use `LorSetConfig`,
which is the same configuration type as `LorMapConfig`:

```c
MyKey *keys = LOR_SET_INIT;
lor_set_init(keys, my_config);
```

String-view sets provide explicit borrowed and owned policies:

```c
LorStringView *borrowed = LOR_SET_INIT;
lor_set_init(
    borrowed, lor_set_config_string_view(LOR_SET_KEY_BORROWED));

LorStringView *owned = LOR_SET_INIT;
lor_set_init(
    owned, lor_set_config_string_view(LOR_SET_KEY_OWNED));
```

Borrowed keys require their source bytes to remain valid and unchanged. Owned
keys copy their bytes and release them on removal, clear, or deinit.

## Mathematical Operations

Set algebra creates a new destination:

```c
int *combined = LOR_SET_INIT;
int *common = LOR_SET_INIT;
int *only_a = LOR_SET_INIT;
int *exclusive = LOR_SET_INIT;

lor_set_union(combined, a, b);
lor_set_intersection(common, a, b);
lor_set_difference(only_a, a, b);
lor_set_symmetric_difference(exclusive, a, b);
```

The destination must be `NULL`. Each operation leaves it `NULL` on failure, so
reuse requires deinitializing the previous result first. Sources are unchanged
and may be the same set.

The result inherits the key policy of a non-empty operand, or otherwise the
first initialized operand. If either compatible operand owns its keys, the
result also owns cloned keys so it remains valid after both sources are
destroyed.

Two non-empty operands must use matching hash, equality, validation, and
context semantics. Borrowed and owned string-view sets are compatible because
they compare keys identically; ownership only controls how the result stores
those keys. Conflicting custom ownership callbacks are rejected.

The operations correspond to Python's set methods:

- `union`: keys in either set.
- `intersection`: keys in both sets.
- `difference`: keys in the left set but not the right.
- `symmetric_difference`: keys in exactly one set.

Relationship predicates are also available:

- `lor_set_equal`
- `lor_set_is_subset`
- `lor_set_is_proper_subset`
- `lor_set_is_superset`
- `lor_set_is_proper_superset`
- `lor_set_is_disjoint`

Predicates return zero for non-empty sets with incompatible key semantics.

## Convenience And Cleanup

The portable API follows the array and map conventions:

```c
int value = 10;
lor_set_add(numbers, value);
lor_set_add_as(numbers, int, 20);
```

`lor_set_add` takes an lvalue of the exact key type. `lor_set_add_as` constructs
a C99 compound literal. When the shared `lor_typeof` and statement-expression
features are available, `LOR_HAS_SET_AUTO` enables inferred expressions:

```c
#if LOR_HAS_SET_AUTO
lor_set_add_auto(numbers, 30);
#endif
```

On GCC and Clang, `LOR_AUTO_SET` releases the set at scope exit. All set
storage and owned string-view keys participate in `LOR_LEAKCHECK`.
