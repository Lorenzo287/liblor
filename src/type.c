// SPDX-License-Identifier: MIT

#include "lor/type.h"

const char *lor_type_kind_name(LorTypeKind kind) {
    switch (kind) {
    case LOR_TYPE_BOOL:
        return "bool";
    case LOR_TYPE_CHAR:
        return "char";
    case LOR_TYPE_SIGNED_CHAR:
        return "signed char";
    case LOR_TYPE_UNSIGNED_CHAR:
        return "unsigned char";
    case LOR_TYPE_SHORT:
        return "short";
    case LOR_TYPE_UNSIGNED_SHORT:
        return "unsigned short";
    case LOR_TYPE_INT:
        return "int";
    case LOR_TYPE_UNSIGNED_INT:
        return "unsigned int";
    case LOR_TYPE_LONG:
        return "long";
    case LOR_TYPE_UNSIGNED_LONG:
        return "unsigned long";
    case LOR_TYPE_LONG_LONG:
        return "long long";
    case LOR_TYPE_UNSIGNED_LONG_LONG:
        return "unsigned long long";
    case LOR_TYPE_FLOAT:
        return "float";
    case LOR_TYPE_DOUBLE:
        return "double";
    case LOR_TYPE_LONG_DOUBLE:
        return "long double";
    case LOR_TYPE_CSTRING:
        return "C string";
    case LOR_TYPE_POINTER:
        return "pointer";
    case LOR_TYPE_STRING_VIEW:
        return "LorStringView";
    case LOR_TYPE_ARENA_CONFIG:
        return "LorArenaConfig";
    case LOR_TYPE_ARENA:
        return "LorArena";
    case LOR_TYPE_ARENA_MARK:
        return "LorArenaMark";
    case LOR_TYPE_SCRATCH:
        return "LorScratch";
    case LOR_TYPE_MMAP:
        return "LorMmap";
    case LOR_TYPE_LEAK_STATS:
        return "LorLeakStats";
    case LOR_TYPE_RANDOM:
        return "LorRandom";
    case LOR_TYPE_OTHER:
        break;
    }

    return "other";
}
