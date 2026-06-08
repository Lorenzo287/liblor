// SPDX-License-Identifier: MIT

#ifndef LOR_PRINT_H
#define LOR_PRINT_H

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include "lor/array.h"  // IWYU pragma: export
#include "lor/map.h"
#include "lor/set.h"  // IWYU pragma: export
#include "lor/status.h"
#include "lor/string.h"
#include "lor/type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LorPrintKind {
    LOR_PRINT_BOOL = 0,
    LOR_PRINT_CHAR,
    LOR_PRINT_SIGNED,
    LOR_PRINT_UNSIGNED,
    LOR_PRINT_FLOATING,
    LOR_PRINT_CSTRING,
    LOR_PRINT_STRING_VIEW,
    LOR_PRINT_POINTER,
    LOR_PRINT_ARENA_CONFIG,
    LOR_PRINT_ARENA,
    LOR_PRINT_ARENA_MARK,
    LOR_PRINT_SCRATCH,
    LOR_PRINT_MMAP,
    LOR_PRINT_LEAK_STATS,
    LOR_PRINT_RANDOM,
    LOR_PRINT_ARRAY,
    LOR_PRINT_SET,
    LOR_PRINT_MAP,
    LOR_PRINT_CUSTOM,
    LOR_PRINT_END
} LorPrintKind;

typedef LorStatus (*LorPrintCustomFn)(FILE *out, const void *value);

typedef struct LorPrintCustom {
    const void *value;
    LorPrintCustomFn function;
} LorPrintCustom;

typedef struct LorPrintSequence {
    const void *data;
    size_t count;
    size_t element_size;
    LorTypeKind element_kind;
    LorPrintCustomFn element_function;
} LorPrintSequence;

typedef struct LorPrintMap {
    const void *data;
    size_t count;
    size_t entry_size;
    size_t key_offset;
    size_t key_size;
    LorTypeKind key_kind;
    LorPrintCustomFn key_function;
    size_t value_offset;
    size_t value_size;
    LorTypeKind value_kind;
    LorPrintCustomFn value_function;
} LorPrintMap;

typedef struct LorPrintValue {
    LorPrintKind kind;
    union {
        int boolean;
        char character;
        intmax_t signed_integer;
        uintmax_t unsigned_integer;
        double floating;
        const char *cstring;
        LorStringView view;
        const void *pointer;
        LorArenaConfig arena_config;
        LorArena arena;
        LorArenaMark arena_mark;
        LorScratch scratch;
        LorMmap mmap;
        LorLeakStats leak_stats;
        LorRandom random;
        LorPrintSequence sequence;
        LorPrintMap map;
        LorPrintCustom custom;
    } as;
} LorPrintValue;

typedef struct LorPrintConfig {
    LorStringView separator;
    LorStringView ending;
} LorPrintConfig;

#define LOR_PRINT_CONFIG_INIT {{" ", 1u}, {"\n", 1u}}

/* Typed marker that replaces the configured ending.

   Markers may appear anywhere in a generic print call and do not participate
   in separator placement. If several are supplied, the last marker wins. */
typedef struct LorPrintEnd {
    LorStringView ending;
} LorPrintEnd;

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_init(LorPrintKind kind) {
    LorPrintValue result;
    result.kind = kind;
    result.as.unsigned_integer = 0;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_bool(int value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_BOOL);
    result.as.boolean = value != 0;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_char(char value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_CHAR);
    result.as.character = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_signed(intmax_t value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_SIGNED);
    result.as.signed_integer = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_unsigned(uintmax_t value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_UNSIGNED);
    result.as.unsigned_integer = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_floating(double value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_FLOATING);
    result.as.floating = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_cstring(const char *value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_CSTRING);
    result.as.cstring = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_view(LorStringView value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_STRING_VIEW);
    result.as.view = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_arena_config(LorArenaConfig value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_ARENA_CONFIG);
    result.as.arena_config = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_arena(LorArena value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_ARENA);
    result.as.arena = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_arena_mark(LorArenaMark value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_ARENA_MARK);
    result.as.arena_mark = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_scratch(LorScratch value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_SCRATCH);
    result.as.scratch = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_mmap(LorMmap value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_MMAP);
    result.as.mmap = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_leak_stats(LorLeakStats value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_LEAK_STATS);
    result.as.leak_stats = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_random(LorRandom value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_RANDOM);
    result.as.random = value;
    return result;
}

// Wraps an object pointer for generic printing with `%p`.
static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_pointer(const void *value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_POINTER);
    result.as.pointer = value;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_sequence_value(
    LorPrintKind kind, const void *data, size_t count, size_t element_size,
    LorTypeKind element_kind, LorPrintCustomFn element_function) {
    LorPrintValue result = lor_print_value_init(kind);
    result.as.sequence.data = data;
    result.as.sequence.count = count;
    result.as.sequence.element_size = element_size;
    result.as.sequence.element_kind = element_kind;
    result.as.sequence.element_function = element_function;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_map_value(
    const void *data, size_t count, size_t entry_size, size_t key_offset,
    size_t key_size, LorTypeKind key_kind, LorPrintCustomFn key_function,
    size_t value_offset, size_t value_size, LorTypeKind value_kind,
    LorPrintCustomFn value_function) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_MAP);
    result.as.map.data = data;
    result.as.map.count = count;
    result.as.map.entry_size = entry_size;
    result.as.map.key_offset = key_offset;
    result.as.map.key_size = key_size;
    result.as.map.key_kind = key_kind;
    result.as.map.key_function = key_function;
    result.as.map.value_offset = value_offset;
    result.as.map.value_size = value_size;
    result.as.map.value_kind = value_kind;
    result.as.map.value_function = value_function;
    return result;
}

// Wraps a user-defined value and its printer callback.
static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_custom(const void *value,
                                             LorPrintCustomFn function) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_CUSTOM);
    result.as.custom.value = value;
    result.as.custom.function = function;
    return result;
}

// Creates an ending marker from a NUL-terminated C string.
static inline LorPrintEnd LOR_MAYBE_UNUSED lor_end(const char *ending) {
    LorPrintEnd result;
    result.ending = lor_sv_from_cstr(ending);
    return result;
}

// Creates an ending marker from an exact-length string view.
static inline LorPrintEnd LOR_MAYBE_UNUSED lor_end_view(LorStringView ending) {
    LorPrintEnd result;
    result.ending = ending;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_end(LorPrintEnd value) {
    LorPrintValue result = lor_print_value_init(LOR_PRINT_END);
    result.as.view = value.ending;
    return result;
}

static inline LorPrintValue LOR_MAYBE_UNUSED lor_print_value_identity(LorPrintValue value) {
    return value;
}

/* Prints an explicit array of tagged values.

   Values are separated by `config.separator` and followed by `config.ending`,
   unless an `LOR_PRINT_END` value replaces it. A zero-value array prints only
   the ending. Returns `LOR_STATUS_SYSTEM_ERROR` on an output failure. */
LorStatus lor_fprint_values(FILE *out, LorPrintConfig config,
                            const LorPrintValue *values, size_t count);

/* C11 generic convenience layer.

   Each expression is evaluated once, although C does not specify evaluation
   order within the generated initializer. Up to 16 arguments are supported.
   Unsupported values fail to compile; wrap object pointers with
   `lor_print_pointer` and user-defined types with `lor_print_custom`. */
#if LOR_HAS_GENERIC_SELECTION
#define LOR_HAS_GENERIC_PRINT 1
#define lor_print_value(value)                        \
    _Generic((value),                                 \
        _Bool: lor_print_value_bool,                  \
        char: lor_print_value_char,                   \
        signed char: lor_print_value_signed,          \
        unsigned char: lor_print_value_unsigned,      \
        short: lor_print_value_signed,                \
        unsigned short: lor_print_value_unsigned,     \
        int: lor_print_value_signed,                  \
        unsigned int: lor_print_value_unsigned,       \
        long: lor_print_value_signed,                 \
        unsigned long: lor_print_value_unsigned,      \
        long long: lor_print_value_signed,            \
        unsigned long long: lor_print_value_unsigned, \
        float: lor_print_value_floating,              \
        double: lor_print_value_floating,             \
        long double: lor_print_value_floating,        \
        char *: lor_print_value_cstring,              \
        const char *: lor_print_value_cstring,        \
        void *: lor_print_pointer,                    \
        const void *: lor_print_pointer,              \
        LorStringView: lor_print_value_view,          \
        LorArenaConfig: lor_print_value_arena_config, \
        LorArena: lor_print_value_arena,              \
        LorArenaMark: lor_print_value_arena_mark,     \
        LorScratch: lor_print_value_scratch,          \
        LorMmap: lor_print_value_mmap,                \
        LorLeakStats: lor_print_value_leak_stats,     \
        LorRandom: lor_print_value_random,            \
        LorPrintEnd: lor_print_value_end,             \
        LorPrintValue: lor_print_value_identity)(value)

/* Container wrappers retain information that a raw typed pointer does not.

   Built-in scalar types, C strings, string views, and concrete liblor value
   structs are inferred automatically. Use the `_with` forms when elements,
   keys, or values are application-defined structures. */
#define lor_print_array_with(array, function)                                 \
    lor_print_sequence_value(LOR_PRINT_ARRAY, (array), lor_array_size(array), \
                             sizeof *(array), lor_type_kind(*(array)), (function))
#define lor_print_array(array) lor_print_array_with((array), NULL)
#define lor_print_set_with(set, function)                             \
    lor_print_sequence_value(LOR_PRINT_SET, (set), lor_set_size(set), \
                             sizeof *(set), lor_type_kind(*(set)), (function))
#define lor_print_set(set) lor_print_set_with((set), NULL)

#define lor_print_map_as_with(map, type, key_function, value_function)     \
    lor_print_map_value((map), lor_map_size(map), sizeof(type),            \
                        offsetof(type, key), sizeof(((type *)0)->key),     \
                        lor_type_kind(((type *)0)->key), (key_function),   \
                        offsetof(type, value), sizeof(((type *)0)->value), \
                        lor_type_kind(((type *)0)->value), (value_function))
#define lor_print_map_as(map, type) lor_print_map_as_with((map), type, NULL, NULL)
#if LOR_HAS_TYPEOF
#define LOR_HAS_PRINT_MAP_AUTO 1
#define lor_print_map_with(map, key_function, value_function)        \
    lor_print_map_as_with((map), lor_typeof(*(map)), (key_function), \
                          (value_function))
#define lor_print_map(map) lor_print_map_with((map), NULL, NULL)
#else
#define LOR_HAS_PRINT_MAP_AUTO 0
#endif

#define lor_print__values_1(a) lor_print_value(a)
#define lor_print__values_2(a, ...) \
    lor_print_value(a), lor_print__values_1(__VA_ARGS__)
#define lor_print__values_3(a, ...) \
    lor_print_value(a), lor_print__values_2(__VA_ARGS__)
#define lor_print__values_4(a, ...) \
    lor_print_value(a), lor_print__values_3(__VA_ARGS__)
#define lor_print__values_5(a, ...) \
    lor_print_value(a), lor_print__values_4(__VA_ARGS__)
#define lor_print__values_6(a, ...) \
    lor_print_value(a), lor_print__values_5(__VA_ARGS__)
#define lor_print__values_7(a, ...) \
    lor_print_value(a), lor_print__values_6(__VA_ARGS__)
#define lor_print__values_8(a, ...) \
    lor_print_value(a), lor_print__values_7(__VA_ARGS__)
#define lor_print__values_9(a, ...) \
    lor_print_value(a), lor_print__values_8(__VA_ARGS__)
#define lor_print__values_10(a, ...) \
    lor_print_value(a), lor_print__values_9(__VA_ARGS__)
#define lor_print__values_11(a, ...) \
    lor_print_value(a), lor_print__values_10(__VA_ARGS__)
#define lor_print__values_12(a, ...) \
    lor_print_value(a), lor_print__values_11(__VA_ARGS__)
#define lor_print__values_13(a, ...) \
    lor_print_value(a), lor_print__values_12(__VA_ARGS__)
#define lor_print__values_14(a, ...) \
    lor_print_value(a), lor_print__values_13(__VA_ARGS__)
#define lor_print__values_15(a, ...) \
    lor_print_value(a), lor_print__values_14(__VA_ARGS__)
#define lor_print__values_16(a, ...) \
    lor_print_value(a), lor_print__values_15(__VA_ARGS__)

#define lor_print__select(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, \
                          _14, _15, _16, name, ...)                               \
    name
#define lor_print__values(...)                                            \
    lor_print__select(                                                    \
        __VA_ARGS__, lor_print__values_16, lor_print__values_15,          \
        lor_print__values_14, lor_print__values_13, lor_print__values_12, \
        lor_print__values_11, lor_print__values_10, lor_print__values_9,  \
        lor_print__values_8, lor_print__values_7, lor_print__values_6,    \
        lor_print__values_5, lor_print__values_4, lor_print__values_3,    \
        lor_print__values_2, lor_print__values_1, 0)(__VA_ARGS__)
#define lor_print__count(...)                                                     \
    lor_print__select(__VA_ARGS__, 16u, 15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, \
                      6u, 5u, 4u, 3u, 2u, 1u, 0u)

#define lor_fprint_with(out, config, ...)                                \
    lor_fprint_values((out), (config),                                   \
                      (LorPrintValue[]){lor_print__values(__VA_ARGS__)}, \
                      lor_print__count(__VA_ARGS__))
#define lor_print_with(config, ...) lor_fprint_with(stdout, (config), __VA_ARGS__)
#define lor_fprint(out, ...) \
    lor_fprint_with((out), ((LorPrintConfig)LOR_PRINT_CONFIG_INIT), __VA_ARGS__)
#define lor_print(...) lor_fprint(stdout, __VA_ARGS__)
#else
#define LOR_HAS_GENERIC_PRINT 0
#endif

#ifdef __cplusplus
}
#endif

#endif
