// SPDX-License-Identifier: MIT

#ifndef LOR_TYPE_H
#define LOR_TYPE_H

#include "lor/features.h"
#include "lor/memory.h"
#include "lor/random.h"
#include "lor/string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LorTypeKind {
    LOR_TYPE_OTHER = 0,
    LOR_TYPE_BOOL,
    LOR_TYPE_CHAR,
    LOR_TYPE_SIGNED_CHAR,
    LOR_TYPE_UNSIGNED_CHAR,
    LOR_TYPE_SHORT,
    LOR_TYPE_UNSIGNED_SHORT,
    LOR_TYPE_INT,
    LOR_TYPE_UNSIGNED_INT,
    LOR_TYPE_LONG,
    LOR_TYPE_UNSIGNED_LONG,
    LOR_TYPE_LONG_LONG,
    LOR_TYPE_UNSIGNED_LONG_LONG,
    LOR_TYPE_FLOAT,
    LOR_TYPE_DOUBLE,
    LOR_TYPE_LONG_DOUBLE,
    LOR_TYPE_CSTRING,
    LOR_TYPE_POINTER,
    LOR_TYPE_STRING_VIEW,
    LOR_TYPE_ARENA_CONFIG,
    LOR_TYPE_ARENA,
    LOR_TYPE_ARENA_MARK,
    LOR_TYPE_SCRATCH,
    LOR_TYPE_MMAP,
    LOR_TYPE_LEAK_STATS,
    LOR_TYPE_RANDOM
} LorTypeKind;

// Returns a stable name for `kind`, or "other".
const char *lor_type_kind_name(LorTypeKind kind);

/* Standard C11 generic type inspection.

   The controlling expression is not evaluated. Typedefs resolve to their
   compatible C type, and unlisted user-defined types return `LOR_TYPE_OTHER`.
   C++ translation units use the explicit `LorTypeKind` API instead. */
#if LOR_HAS_GENERIC_SELECTION
#define lor_type_kind(value)                             \
    _Generic((value),                                    \
        _Bool: LOR_TYPE_BOOL,                            \
        char: LOR_TYPE_CHAR,                             \
        signed char: LOR_TYPE_SIGNED_CHAR,               \
        unsigned char: LOR_TYPE_UNSIGNED_CHAR,           \
        short: LOR_TYPE_SHORT,                           \
        unsigned short: LOR_TYPE_UNSIGNED_SHORT,         \
        int: LOR_TYPE_INT,                               \
        unsigned int: LOR_TYPE_UNSIGNED_INT,             \
        long: LOR_TYPE_LONG,                             \
        unsigned long: LOR_TYPE_UNSIGNED_LONG,           \
        long long: LOR_TYPE_LONG_LONG,                   \
        unsigned long long: LOR_TYPE_UNSIGNED_LONG_LONG, \
        float: LOR_TYPE_FLOAT,                           \
        double: LOR_TYPE_DOUBLE,                         \
        long double: LOR_TYPE_LONG_DOUBLE,               \
        char *: LOR_TYPE_CSTRING,                        \
        const char *: LOR_TYPE_CSTRING,                  \
        volatile char *: LOR_TYPE_CSTRING,               \
        const volatile char *: LOR_TYPE_CSTRING,         \
        void *: LOR_TYPE_POINTER,                        \
        const void *: LOR_TYPE_POINTER,                  \
        volatile void *: LOR_TYPE_POINTER,               \
        const volatile void *: LOR_TYPE_POINTER,         \
        LorStringView: LOR_TYPE_STRING_VIEW,             \
        LorArenaConfig: LOR_TYPE_ARENA_CONFIG,           \
        LorArena: LOR_TYPE_ARENA,                        \
        LorArenaMark: LOR_TYPE_ARENA_MARK,               \
        LorScratch: LOR_TYPE_SCRATCH,                    \
        LorMmap: LOR_TYPE_MMAP,                          \
        LorLeakStats: LOR_TYPE_LEAK_STATS,               \
        LorRandom: LOR_TYPE_RANDOM,                      \
        default: LOR_TYPE_OTHER)
#define lor_type_name(value) lor_type_kind_name(lor_type_kind(value))
#endif

#ifdef __cplusplus
}
#endif

#endif
