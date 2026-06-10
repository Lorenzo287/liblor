# Hash Maps

liblor hash maps store typed entries in a dense array and use a separate
open-addressed index for lookup. Entries must begin with a field named `key`.
The `LOR_MAP_ENTRY` helper defines the common key/value shape:

```c
typedef LOR_MAP_ENTRY(int, const char *) IdName;

IdName *names = LOR_MAP_INIT;
lor_map_put_as(names, IdName, 10, "ten");

int key = 10;
IdName *entry = lor_map_find(names, key);
if (entry != NULL)
    puts(entry->value);

lor_map_deinit(&names);
```

`lor_map_capacity(map)` returns the allocated entry capacity.
`lor_map_entry_size(map)` and `lor_map_key_size(map)` expose the configured
layout.

Maps are directly iterable:

```c
for (size_t i = 0; i < lor_map_size(names); ++i)
    printf("%d = %s\n", names[i].key, names[i].value);
```

Iteration order is unspecified and may change after insertion or removal.
Pointers to entries may be invalidated by insertion, reserve, or removal.
Values may be modified in place, but keys must not be changed because their
hash-table positions depend on their contents.

## Key Behavior

A zero-initialized map uses byte hashing and equality. The complete object
representation of the first `sizeof map->key` bytes is the key:

```c
typedef LOR_MAP_ENTRY(unsigned, float) Score;
Score *scores = LOR_MAP_INIT;
```

This works well for integer and enum keys. Pointer keys compare pointer
identity. Structs with padding, floating-point keys, strings, and keys needing
semantic comparison should use a custom `LorMapConfig`.

Custom maps must be initialized before use:

```c
LorMapConfig config = {
    .hash = my_hash,
    .equal = my_equal,
    .context = my_context,
};

MyEntry *map = LOR_MAP_INIT;
lor_map_init(map, config);
```

`hash` and `equal` receive a pointer to the key, its byte size, and the
unchanged context pointer. Equal keys must always produce equal hashes. An
optional `key_valid` callback rejects malformed keys before hashing.

`clone` and `drop` are optional paired callbacks for keys that own resources.
`clone` prepares a stored key and may return a `LorStatus`; `drop` releases a
successfully cloned key. Map values are always shallow. If values own strings,
arrays, pointers, or other resources, callers must release those values before
removing or clearing their entries.

`lor_map_get_config` exposes the stored configuration for generic facades such
as sets. `lor_map_config_equal` checks exact callback and context identity; it
does not attempt to prove that different functions implement equivalent key
semantics.

## String-View Keys

String views can be compared by content with either borrowed or owned storage:

```c
typedef LOR_MAP_ENTRY(LorStringView, size_t) WordCount;

WordCount *borrowed = LOR_MAP_INIT;
lor_map_init(
    borrowed, lor_map_config_string_view(LOR_MAP_KEY_BORROWED));

WordCount *owned = LOR_MAP_INIT;
lor_map_init(
    owned, lor_map_config_string_view(LOR_MAP_KEY_OWNED));
```

A borrowed map copies each `LorStringView` structure but not its bytes. The
source storage must remain valid and unchanged while the key is present.

An owned map copies the bytes during insertion. Its keys remain valid after the
source buffer is changed or released, and copied bytes are released on key
replacement, removal, clear, or deinit. This policy also accepts views into
`LorString`, C strings, mapped files, and parsing buffers.

## Insertion Forms

The portable forms follow the dynamic-array convention:

```c
WordCount entry = {.key = word, .value = 1};
lor_map_set(counts, entry);

lor_map_set_as(counts, WordCount, .key = word, .value = 1);
lor_map_put_as(counts, WordCount, word, 1);
```

`lor_map_set` takes an lvalue of the exact entry type. `lor_map_set_as` creates
a C99 compound literal and supports entries with fields beyond `key` and
`value`. `lor_map_put_as` is the concise common key/value form.

`lor_map_contains(map, key)` returns non-zero when a key is present.
`lor_map_remove(map, key)` removes an entry and returns non-zero.

On GCC and Clang, `LOR_HAS_MAP_AUTO` enables inferred convenience operations:

```c
#if LOR_HAS_MAP_AUTO
lor_map_put_auto(scores, 10u, 9.5f);
Score *score = lor_map_find_auto(scores, 10u);
#endif
```

These use the shared `lor_typeof` and statement-expression feature checks,
evaluate supplied expressions once, and perform normal assignment conversions.
Use the portable forms when support for other compilers matters.

## Failure And Lifetime

`lor_map_init`, `lor_map_reserve`, and insertion return `LorStatus`. Allocation,
overflow, invalid layouts, and key-clone failures leave existing logical
entries unchanged. Lookup and removal report absence without setting global
error state.

`lor_map_clear` releases owned keys but retains allocations and configuration.
`lor_map_deinit` also releases the dense storage and bucket index and resets the
handle to `NULL`.

On GCC and Clang, `LOR_AUTO_MAP` performs scope-exit cleanup:

```c
LOR_AUTO_MAP WordCount *counts = LOR_MAP_INIT;
```

All map allocations, including copied string keys, participate in
`LOR_LEAKCHECK`.
