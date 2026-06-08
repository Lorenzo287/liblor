// SPDX-License-Identifier: MIT

#include "lor/print.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
typedef bool LorPrintBoolValue;
#else
typedef _Bool LorPrintBoolValue;
#endif

static LorStatus lor_print__write_view(FILE *out, LorStringView view) {
    if (!lor_sv_is_valid(view)) return LOR_STATUS_INVALID_ARGUMENT;
    if (view.size == 0) return LOR_STATUS_OK;
    if (fwrite(view.data, 1, view.size, out) != view.size)
        return LOR_STATUS_SYSTEM_ERROR;
    return LOR_STATUS_OK;
}

static LorStatus lor_print__write_quoted_view(FILE *out, LorStringView view) {
    if (!lor_sv_is_valid(view)) return LOR_STATUS_INVALID_ARGUMENT;
    if (fputc('"', out) == EOF) return LOR_STATUS_SYSTEM_ERROR;

    for (size_t i = 0; i < view.size; ++i) {
        unsigned char byte = (unsigned char)view.data[i];
        int written;

        switch (byte) {
        case '\\':
            written = fputs("\\\\", out);
            break;
        case '"':
            written = fputs("\\\"", out);
            break;
        case '\n':
            written = fputs("\\n", out);
            break;
        case '\r':
            written = fputs("\\r", out);
            break;
        case '\t':
            written = fputs("\\t", out);
            break;
        default:
            written = isprint(byte) ? fputc(byte, out)
                                    : fprintf(out, "\\x%02x", (unsigned)byte);
            break;
        }

        if (written < 0) return LOR_STATUS_SYSTEM_ERROR;
    }

    return fputc('"', out) == EOF ? LOR_STATUS_SYSTEM_ERROR : LOR_STATUS_OK;
}

static const char *lor_print__arena_backend_name(LorArenaBackend backend) {
    switch (backend) {
    case LOR_ARENA_BACKEND_HEAP:
        return "heap";
    case LOR_ARENA_BACKEND_VIRTUAL:
        return "virtual";
    }
    return "unknown";
}

static int lor_print__type_size_valid(LorTypeKind kind, size_t size) {
    switch (kind) {
    case LOR_TYPE_BOOL:
        return size == sizeof(LorPrintBoolValue);
    case LOR_TYPE_CHAR:
        return size == sizeof(char);
    case LOR_TYPE_SIGNED_CHAR:
        return size == sizeof(signed char);
    case LOR_TYPE_UNSIGNED_CHAR:
        return size == sizeof(unsigned char);
    case LOR_TYPE_SHORT:
        return size == sizeof(short);
    case LOR_TYPE_UNSIGNED_SHORT:
        return size == sizeof(unsigned short);
    case LOR_TYPE_INT:
        return size == sizeof(int);
    case LOR_TYPE_UNSIGNED_INT:
        return size == sizeof(unsigned int);
    case LOR_TYPE_LONG:
        return size == sizeof(long);
    case LOR_TYPE_UNSIGNED_LONG:
        return size == sizeof(unsigned long);
    case LOR_TYPE_LONG_LONG:
        return size == sizeof(long long);
    case LOR_TYPE_UNSIGNED_LONG_LONG:
        return size == sizeof(unsigned long long);
    case LOR_TYPE_FLOAT:
        return size == sizeof(float);
    case LOR_TYPE_DOUBLE:
        return size == sizeof(double);
    case LOR_TYPE_LONG_DOUBLE:
        return size == sizeof(long double);
    case LOR_TYPE_CSTRING:
        return size == sizeof(char *);
    case LOR_TYPE_POINTER:
        return size == sizeof(void *);
    case LOR_TYPE_STRING_VIEW:
        return size == sizeof(LorStringView);
    case LOR_TYPE_ARENA_CONFIG:
        return size == sizeof(LorArenaConfig);
    case LOR_TYPE_ARENA:
        return size == sizeof(LorArena);
    case LOR_TYPE_ARENA_MARK:
        return size == sizeof(LorArenaMark);
    case LOR_TYPE_SCRATCH:
        return size == sizeof(LorScratch);
    case LOR_TYPE_MMAP:
        return size == sizeof(LorMmap);
    case LOR_TYPE_LEAK_STATS:
        return size == sizeof(LorLeakStats);
    case LOR_TYPE_RANDOM:
        return size == sizeof(LorRandom);
    case LOR_TYPE_OTHER:
        return 0;
    }
    return 0;
}

static int lor_print__typed_value_valid(const void *value, size_t size,
                                        LorTypeKind kind,
                                        LorPrintCustomFn function) {
    if (value == NULL) return 0;
    if (function != NULL) return 1;
    if (!lor_print__type_size_valid(kind, size)) return 0;
    if (kind == LOR_TYPE_STRING_VIEW)
        return lor_sv_is_valid(*(const LorStringView *)value);
    return 1;
}

static int lor_print__sequence_valid(LorPrintSequence sequence) {
    if ((sequence.data == NULL && sequence.count != 0) ||
        sequence.element_size == 0 ||
        sequence.count > SIZE_MAX / sequence.element_size)
        return 0;

    const unsigned char *data = (const unsigned char *)sequence.data;
    for (size_t i = 0; i < sequence.count; ++i) {
        if (!lor_print__typed_value_valid(
                data + i * sequence.element_size, sequence.element_size,
                sequence.element_kind, sequence.element_function))
            return 0;
    }
    return 1;
}

static int lor_print__map_valid(LorPrintMap map) {
    if ((map.data == NULL && map.count != 0) || map.entry_size == 0 ||
        map.count > SIZE_MAX / map.entry_size || map.key_offset > map.entry_size ||
        map.key_size > map.entry_size - map.key_offset ||
        map.value_offset > map.entry_size ||
        map.value_size > map.entry_size - map.value_offset)
        return 0;

    const unsigned char *data = (const unsigned char *)map.data;
    for (size_t i = 0; i < map.count; ++i) {
        const unsigned char *entry = data + i * map.entry_size;
        if (!lor_print__typed_value_valid(entry + map.key_offset, map.key_size,
                                          map.key_kind, map.key_function) ||
            !lor_print__typed_value_valid(entry + map.value_offset, map.value_size,
                                          map.value_kind, map.value_function))
            return 0;
    }
    return 1;
}

static int lor_print__value_valid(LorPrintValue value) {
    switch (value.kind) {
    case LOR_PRINT_STRING_VIEW:
    case LOR_PRINT_END:
        return lor_sv_is_valid(value.as.view);
    case LOR_PRINT_CUSTOM:
        return value.as.custom.function != NULL;
    case LOR_PRINT_ARRAY:
    case LOR_PRINT_SET:
        return lor_print__sequence_valid(value.as.sequence);
    case LOR_PRINT_MAP:
        return lor_print__map_valid(value.as.map);
    case LOR_PRINT_BOOL:
    case LOR_PRINT_CHAR:
    case LOR_PRINT_SIGNED:
    case LOR_PRINT_UNSIGNED:
    case LOR_PRINT_FLOATING:
    case LOR_PRINT_CSTRING:
    case LOR_PRINT_POINTER:
    case LOR_PRINT_ARENA_CONFIG:
    case LOR_PRINT_ARENA:
    case LOR_PRINT_ARENA_MARK:
    case LOR_PRINT_SCRATCH:
    case LOR_PRINT_MMAP:
    case LOR_PRINT_LEAK_STATS:
    case LOR_PRINT_RANDOM:
        return 1;
    }

    return 0;
}

static LorStatus lor_print__write_typed_value(FILE *out, const void *value,
                                              size_t size, LorTypeKind kind,
                                              LorPrintCustomFn function,
                                              int nested) {
    int written;

    if (function != NULL) return function(out, value);
    if (!lor_print__type_size_valid(kind, size)) return LOR_STATUS_INVALID_ARGUMENT;

    switch (kind) {
    case LOR_TYPE_BOOL:
        written = fputs(*(const LorPrintBoolValue *)value ? "true" : "false", out);
        break;
    case LOR_TYPE_CHAR:
        written = fputc((unsigned char)*(const char *)value, out);
        break;
    case LOR_TYPE_SIGNED_CHAR:
        written = fprintf(out, "%hhd", *(const signed char *)value);
        break;
    case LOR_TYPE_UNSIGNED_CHAR:
        written = fprintf(out, "%hhu", *(const unsigned char *)value);
        break;
    case LOR_TYPE_SHORT:
        written = fprintf(out, "%hd", *(const short *)value);
        break;
    case LOR_TYPE_UNSIGNED_SHORT:
        written = fprintf(out, "%hu", *(const unsigned short *)value);
        break;
    case LOR_TYPE_INT:
        written = fprintf(out, "%d", *(const int *)value);
        break;
    case LOR_TYPE_UNSIGNED_INT:
        written = fprintf(out, "%u", *(const unsigned int *)value);
        break;
    case LOR_TYPE_LONG:
        written = fprintf(out, "%ld", *(const long *)value);
        break;
    case LOR_TYPE_UNSIGNED_LONG:
        written = fprintf(out, "%lu", *(const unsigned long *)value);
        break;
    case LOR_TYPE_LONG_LONG:
        written = fprintf(out, "%lld", *(const long long *)value);
        break;
    case LOR_TYPE_UNSIGNED_LONG_LONG:
        written = fprintf(out, "%llu", *(const unsigned long long *)value);
        break;
    case LOR_TYPE_FLOAT:
        written = fprintf(out, "%g", (double)*(const float *)value);
        break;
    case LOR_TYPE_DOUBLE:
        written = fprintf(out, "%g", *(const double *)value);
        break;
    case LOR_TYPE_LONG_DOUBLE:
        written = fprintf(out, "%g", (double)*(const long double *)value);
        break;
    case LOR_TYPE_CSTRING: {
        const char *text;
        memcpy(&text, value, sizeof(text));
        if (text == NULL) {
            written = fputs("(null)", out);
            break;
        }
        if (nested) return lor_print__write_quoted_view(out, lor_sv_from_cstr(text));
        written = fputs(text, out);
        break;
    }
    case LOR_TYPE_POINTER: {
        void *pointer;
        memcpy(&pointer, value, sizeof(pointer));
        written = fprintf(out, "%p", pointer);
        break;
    }
    case LOR_TYPE_STRING_VIEW: {
        LorStringView view = *(const LorStringView *)value;
        return nested ? lor_print__write_quoted_view(out, view)
                      : lor_print__write_view(out, view);
    }
    case LOR_TYPE_ARENA_CONFIG: {
        const LorArenaConfig *config = (const LorArenaConfig *)value;
        written =
            fprintf(out,
                    "LorArenaConfig(backend=%s, block_size=%zu, reserve_size=%zu, "
                    "commit_size=%zu)",
                    lor_print__arena_backend_name(config->backend),
                    config->block_size, config->reserve_size, config->commit_size);
        break;
    }
    case LOR_TYPE_ARENA: {
        const LorArena *arena = (const LorArena *)value;
        written = fprintf(
            out, "LorArena(backend=%s, used=%zu, capacity=%zu, committed=%zu)",
            lor_print__arena_backend_name(arena->backend), lor_arena_used(arena),
            lor_arena_capacity(arena), lor_arena_committed(arena));
        break;
    }
    case LOR_TYPE_ARENA_MARK: {
        const LorArenaMark *mark = (const LorArenaMark *)value;
        written = fprintf(out, "LorArenaMark(block=%p, used=%zu)",
                          (void *)mark->block, mark->used);
        break;
    }
    case LOR_TYPE_SCRATCH: {
        const LorScratch *scratch = (const LorScratch *)value;
        written = fprintf(out, "LorScratch(arena=%p, mark_used=%zu)",
                          (void *)scratch->arena, scratch->mark.used);
        break;
    }
    case LOR_TYPE_MMAP: {
        const LorMmap *map = (const LorMmap *)value;
        written = fprintf(out, "LorMmap(data=%p, size=%zu)", map->data, map->size);
        break;
    }
    case LOR_TYPE_LEAK_STATS: {
        const LorLeakStats *stats = (const LorLeakStats *)value;
        written =
            fprintf(out,
                    "LorLeakStats(heap_count=%zu, heap_bytes=%zu, arena_count=%zu, "
                    "mmap_count=%zu, mmap_bytes=%zu)",
                    stats->heap_count, stats->heap_bytes, stats->arena_count,
                    stats->mmap_count, stats->mmap_bytes);
        break;
    }
    case LOR_TYPE_RANDOM: {
        const LorRandom *random = (const LorRandom *)value;
        written = fprintf(out, "LorRandom(state=%" PRIu64 ", increment=%" PRIu64 ")",
                          random->state, random->increment);
        break;
    }
    case LOR_TYPE_OTHER:
    default:
        return LOR_STATUS_INVALID_ARGUMENT;
    }

    return written < 0 ? LOR_STATUS_SYSTEM_ERROR : LOR_STATUS_OK;
}

static LorStatus lor_print__write_sequence(FILE *out, LorPrintSequence sequence,
                                           int is_set) {
    if (sequence.count == 0)
        return fputs(is_set ? "set()" : "[]", out) < 0 ? LOR_STATUS_SYSTEM_ERROR
                                                       : LOR_STATUS_OK;

    if (fputc(is_set ? '{' : '[', out) == EOF) return LOR_STATUS_SYSTEM_ERROR;
    const unsigned char *data = (const unsigned char *)sequence.data;
    for (size_t i = 0; i < sequence.count; ++i) {
        if (i != 0 && fputs(", ", out) < 0) return LOR_STATUS_SYSTEM_ERROR;
        LorStatus status = lor_print__write_typed_value(
            out, data + i * sequence.element_size, sequence.element_size,
            sequence.element_kind, sequence.element_function, 1);
        if (status != LOR_STATUS_OK) return status;
    }
    return fputc(is_set ? '}' : ']', out) == EOF ? LOR_STATUS_SYSTEM_ERROR
                                                 : LOR_STATUS_OK;
}

static LorStatus lor_print__write_map(FILE *out, LorPrintMap map) {
    if (fputc('{', out) == EOF) return LOR_STATUS_SYSTEM_ERROR;
    const unsigned char *data = (const unsigned char *)map.data;
    for (size_t i = 0; i < map.count; ++i) {
        if (i != 0 && fputs(", ", out) < 0) return LOR_STATUS_SYSTEM_ERROR;
        const unsigned char *entry = data + i * map.entry_size;

        LorStatus status =
            lor_print__write_typed_value(out, entry + map.key_offset, map.key_size,
                                         map.key_kind, map.key_function, 1);
        if (status != LOR_STATUS_OK) return status;
        if (fputs(": ", out) < 0) return LOR_STATUS_SYSTEM_ERROR;
        status = lor_print__write_typed_value(out, entry + map.value_offset,
                                              map.value_size, map.value_kind,
                                              map.value_function, 1);
        if (status != LOR_STATUS_OK) return status;
    }
    return fputc('}', out) == EOF ? LOR_STATUS_SYSTEM_ERROR : LOR_STATUS_OK;
}

static LorStatus lor_print__write_value(FILE *out, LorPrintValue value) {
    int written;

    switch (value.kind) {
    case LOR_PRINT_BOOL:
        written = fputs(value.as.boolean ? "true" : "false", out);
        break;
    case LOR_PRINT_CHAR:
        written = fputc((unsigned char)value.as.character, out);
        break;
    case LOR_PRINT_SIGNED:
        written = fprintf(out, "%" PRIdMAX, value.as.signed_integer);
        break;
    case LOR_PRINT_UNSIGNED:
        written = fprintf(out, "%" PRIuMAX, value.as.unsigned_integer);
        break;
    case LOR_PRINT_FLOATING:
        written = fprintf(out, "%g", value.as.floating);
        break;
    case LOR_PRINT_CSTRING:
        written = fputs(value.as.cstring != NULL ? value.as.cstring : "(null)", out);
        break;
    case LOR_PRINT_STRING_VIEW:
        return lor_print__write_view(out, value.as.view);
    case LOR_PRINT_POINTER:
        written = fprintf(out, "%p", (void *)value.as.pointer);
        break;
    case LOR_PRINT_ARENA_CONFIG:
        return lor_print__write_typed_value(out, &value.as.arena_config,
                                            sizeof(value.as.arena_config),
                                            LOR_TYPE_ARENA_CONFIG, NULL, 0);
    case LOR_PRINT_ARENA:
        return lor_print__write_typed_value(
            out, &value.as.arena, sizeof(value.as.arena), LOR_TYPE_ARENA, NULL, 0);
    case LOR_PRINT_ARENA_MARK:
        return lor_print__write_typed_value(out, &value.as.arena_mark,
                                            sizeof(value.as.arena_mark),
                                            LOR_TYPE_ARENA_MARK, NULL, 0);
    case LOR_PRINT_SCRATCH:
        return lor_print__write_typed_value(out, &value.as.scratch,
                                            sizeof(value.as.scratch),
                                            LOR_TYPE_SCRATCH, NULL, 0);
    case LOR_PRINT_MMAP:
        return lor_print__write_typed_value(
            out, &value.as.mmap, sizeof(value.as.mmap), LOR_TYPE_MMAP, NULL, 0);
    case LOR_PRINT_LEAK_STATS:
        return lor_print__write_typed_value(out, &value.as.leak_stats,
                                            sizeof(value.as.leak_stats),
                                            LOR_TYPE_LEAK_STATS, NULL, 0);
    case LOR_PRINT_RANDOM:
        return lor_print__write_typed_value(out, &value.as.random,
                                            sizeof(value.as.random), LOR_TYPE_RANDOM,
                                            NULL, 0);
    case LOR_PRINT_ARRAY:
        return lor_print__write_sequence(out, value.as.sequence, 0);
    case LOR_PRINT_SET:
        return lor_print__write_sequence(out, value.as.sequence, 1);
    case LOR_PRINT_MAP:
        return lor_print__write_map(out, value.as.map);
    case LOR_PRINT_CUSTOM:
        return value.as.custom.function(out, value.as.custom.value);
    case LOR_PRINT_END:
        return LOR_STATUS_OK;
    default:
        return LOR_STATUS_INVALID_ARGUMENT;
    }

    return written < 0 ? LOR_STATUS_SYSTEM_ERROR : LOR_STATUS_OK;
}

LorStatus lor_fprint_values(FILE *out, LorPrintConfig config,
                            const LorPrintValue *values, size_t count) {
    if (out == NULL || !lor_sv_is_valid(config.separator) ||
        !lor_sv_is_valid(config.ending) || (values == NULL && count != 0))
        return LOR_STATUS_INVALID_ARGUMENT;

    LorStringView ending = config.ending;
    for (size_t i = 0; i < count; ++i) {
        if (!lor_print__value_valid(values[i])) return LOR_STATUS_INVALID_ARGUMENT;
        if (values[i].kind == LOR_PRINT_END) ending = values[i].as.view;
    }

    size_t printed_count = 0;
    for (size_t i = 0; i < count; ++i) {
        if (values[i].kind == LOR_PRINT_END) continue;

        if (printed_count != 0) {
            LorStatus status = lor_print__write_view(out, config.separator);
            if (status != LOR_STATUS_OK) return status;
        }

        LorStatus status = lor_print__write_value(out, values[i]);
        if (status != LOR_STATUS_OK) return status;
        ++printed_count;
    }

    return lor_print__write_view(out, ending);
}
