// SPDX-License-Identifier: MIT

#include "lor/trace.h"

#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#if defined(LOR_TRACE_AUTO)
#include <dbghelp.h>
#endif
#else
#include <unistd.h>
#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#define LOR_TRACE__NOINSTRUMENT __attribute__((no_instrument_function))
#else
#define LOR_TRACE__NOINSTRUMENT
#endif

#if defined(_MSC_VER)
#define LOR_TRACE__THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define LOR_TRACE__THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
#define LOR_TRACE__THREAD_LOCAL __thread
#else
#define LOR_TRACE__THREAD_LOCAL
#endif

#if LOR_HAS_MAX_ALIGN_T
typedef max_align_t LorTraceAlignment;
#else
typedef union LorTraceAlignment {
    void *pointer;
    long double long_double;
    long long long_long;
} LorTraceAlignment;
#endif

typedef enum LorTraceRecordType {
    LOR_TRACE__RECORD_BEGIN = 1,
    LOR_TRACE__RECORD_END = 2,
    LOR_TRACE__RECORD_INSTANT = 3,
    LOR_TRACE__RECORD_COUNTER = 4,
    LOR_TRACE__RECORD_AUTO_BEGIN = 5
} LorTraceRecordType;

typedef struct LorTraceRecordHeader {
    uint64_t timestamp_ns;
    uint64_t value;
    uint32_t name_size;
    uint8_t type;
    uint8_t reserved[3];
} LorTraceRecordHeader;

typedef struct LorTraceImpl {
    FILE *file;
    atomic_flag lock;
    uint64_t origin_ns;
    uint64_t process_id;
    uint64_t next_thread_id;
    size_t active_threads;
    size_t buffer_size;
    LorTraceBufferMode buffer_mode;
    int first_event;
    int failed;
    int closed;
#if defined(_WIN32)
    uint64_t clock_frequency;
#if defined(LOR_TRACE_AUTO)
    HANDLE symbol_process;
    int symbols_ready;
#endif
#elif defined(__APPLE__)
    mach_timebase_info_data_t timebase;
#endif
} LorTraceImpl;

typedef struct LorTraceThreadImpl {
    LorTraceImpl *trace;
    unsigned char *buffer;
    size_t capacity;
    size_t used;
    size_t depth;
    size_t suppressed_depth;
    size_t reserved_ends;
    uint64_t thread_id;
    uint64_t recorded_events;
    uint64_t dropped_events;
} LorTraceThreadImpl;

static LOR_TRACE__THREAD_LOCAL LorTraceThread *lor_trace__auto_thread = NULL;
static LOR_TRACE__THREAD_LOCAL int lor_trace__auto_enabled = 0;

static LOR_TRACE__NOINSTRUMENT void lor_trace__lock(LorTraceImpl *trace) {
    while (atomic_flag_test_and_set_explicit(&trace->lock, memory_order_acquire)) {}
}

static LOR_TRACE__NOINSTRUMENT void lor_trace__unlock(LorTraceImpl *trace) {
    atomic_flag_clear_explicit(&trace->lock, memory_order_release);
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_bytes(LorTraceImpl *trace,
                                                          const void *data,
                                                          size_t size) {
    if (size == 0) return 1;
    if (fwrite(data, 1, size, trace->file) == size) return 1;
    trace->failed = 1;
    return 0;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_literal(LorTraceImpl *trace,
                                                            const char *text) {
    return lor_trace__write_bytes(trace, text, strlen(text));
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_char(LorTraceImpl *trace,
                                                         int value) {
    if (fputc(value, trace->file) != EOF) return 1;
    trace->failed = 1;
    return 0;
}

static LOR_TRACE__NOINSTRUMENT FILE *lor_trace__open_file(const char *path) {
#if defined(_WIN32) && defined(_MSC_VER)
    FILE *file = NULL;
    return fopen_s(&file, path, "wb") == 0 ? file : NULL;
#else
    return fopen(path, "wb");
#endif
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_json_string(LorTraceImpl *trace,
                                                                const char *text,
                                                                size_t size) {
    static const char hex[] = "0123456789abcdef";
    if (!lor_trace__write_char(trace, '"')) return 0;

    for (size_t i = 0; i < size; ++i) {
        unsigned char byte = (unsigned char)text[i];
        switch (byte) {
        case '"':
            if (!lor_trace__write_literal(trace, "\\\"")) return 0;
            break;
        case '\\':
            if (!lor_trace__write_literal(trace, "\\\\")) return 0;
            break;
        case '\b':
            if (!lor_trace__write_literal(trace, "\\b")) return 0;
            break;
        case '\f':
            if (!lor_trace__write_literal(trace, "\\f")) return 0;
            break;
        case '\n':
            if (!lor_trace__write_literal(trace, "\\n")) return 0;
            break;
        case '\r':
            if (!lor_trace__write_literal(trace, "\\r")) return 0;
            break;
        case '\t':
            if (!lor_trace__write_literal(trace, "\\t")) return 0;
            break;
        default:
            if (byte < 0x20u) {
                char escaped[] = {
                    '\\', 'u', '0', '0', hex[byte >> 4u], hex[byte & 0x0fu]};
                if (!lor_trace__write_bytes(trace, escaped, sizeof(escaped)))
                    return 0;
            } else if (!lor_trace__write_char(trace, byte))
                return 0;
        }
    }

    return lor_trace__write_char(trace, '"');
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_u64(LorTraceImpl *trace,
                                                        uint64_t value) {
    if (fprintf(trace->file, "%" PRIu64, value) >= 0) return 1;
    trace->failed = 1;
    return 0;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_i64(LorTraceImpl *trace,
                                                        int64_t value) {
    if (fprintf(trace->file, "%" PRId64, value) >= 0) return 1;
    trace->failed = 1;
    return 0;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_timestamp(LorTraceImpl *trace,
                                                              uint64_t nanoseconds) {
    uint64_t microseconds = nanoseconds / UINT64_C(1000);
    unsigned int remainder = (unsigned int)(nanoseconds % UINT64_C(1000));
    if (!lor_trace__write_u64(trace, microseconds)) return 0;
    if (remainder == 0) return 1;
    if (fprintf(trace->file, ".%03u", remainder) >= 0) return 1;
    trace->failed = 1;
    return 0;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__event_separator(LorTraceImpl *trace) {
    if (trace->first_event) {
        trace->first_event = 0;
        return 1;
    }
    return lor_trace__write_literal(trace, ",\n");
}

static LOR_TRACE__NOINSTRUMENT uint64_t lor_trace__process_id(void) {
#if defined(_WIN32)
    return (uint64_t)GetCurrentProcessId();
#elif defined(__unix__) || defined(__APPLE__)
    return (uint64_t)getpid();
#else
    return UINT64_C(1);
#endif
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__clock_init(LorTraceImpl *trace) {
#if defined(_WIN32)
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) return 0;
    trace->clock_frequency = (uint64_t)frequency.QuadPart;
#elif defined(__APPLE__)
    if (mach_timebase_info(&trace->timebase) != KERN_SUCCESS ||
        trace->timebase.denom == 0)
        return 0;
#endif
    return 1;
}

static LOR_TRACE__NOINSTRUMENT uint64_t
lor_trace__now_ns(const LorTraceImpl *trace) {
#if defined(_WIN32)
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter) || counter.QuadPart < 0) return 0;
    uint64_t ticks = (uint64_t)counter.QuadPart;
    uint64_t seconds = ticks / trace->clock_frequency;
    uint64_t remainder = ticks % trace->clock_frequency;
    if (seconds > UINT64_MAX / UINT64_C(1000000000)) return UINT64_MAX;
    return seconds * UINT64_C(1000000000) +
           remainder * UINT64_C(1000000000) / trace->clock_frequency;
#elif defined(__APPLE__)
    uint64_t ticks = mach_absolute_time();
    uint64_t quotient = ticks / trace->timebase.denom;
    uint64_t remainder = ticks % trace->timebase.denom;
    if (quotient > UINT64_MAX / trace->timebase.numer) return UINT64_MAX;
    return quotient * trace->timebase.numer +
           remainder * trace->timebase.numer / trace->timebase.denom;
#elif defined(CLOCK_MONOTONIC)
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0 || value.tv_sec < 0) return 0;
    if ((uint64_t)value.tv_sec > UINT64_MAX / UINT64_C(1000000000))
        return UINT64_MAX;
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
#else
    struct timespec value;
    if (timespec_get(&value, TIME_UTC) != TIME_UTC || value.tv_sec < 0) return 0;
    if ((uint64_t)value.tv_sec > UINT64_MAX / UINT64_C(1000000000))
        return UINT64_MAX;
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
#endif
}

static LOR_TRACE__NOINSTRUMENT uint64_t
lor_trace__timestamp(const LorTraceImpl *trace) {
    uint64_t now = lor_trace__now_ns(trace);
    return now >= trace->origin_ns ? now - trace->origin_ns : 0;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_metadata_locked(
    LorTraceImpl *trace, const char *metadata_name, uint64_t thread_id,
    const char *value) {
    size_t value_size = strlen(value);
    return lor_trace__event_separator(trace) &&
           lor_trace__write_literal(trace, "{\"name\":") &&
           lor_trace__write_json_string(trace, metadata_name,
                                        strlen(metadata_name)) &&
           lor_trace__write_literal(trace, ",\"ph\":\"M\",\"pid\":") &&
           lor_trace__write_u64(trace, trace->process_id) &&
           lor_trace__write_literal(trace, ",\"tid\":") &&
           lor_trace__write_u64(trace, thread_id) &&
           lor_trace__write_literal(trace, ",\"args\":{\"name\":") &&
           lor_trace__write_json_string(trace, value, value_size) &&
           lor_trace__write_literal(trace, "}}");
}

static LOR_TRACE__NOINSTRUMENT size_t lor_trace__auto_name(LorTraceImpl *trace,
                                                           uintptr_t address,
                                                           char *buffer,
                                                           size_t capacity) {
#if defined(_WIN32) && defined(LOR_TRACE_AUTO)
    if (trace->symbols_ready) {
        union {
            LorTraceAlignment alignment;
            unsigned char bytes[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        } storage;
        memset(&storage, 0, sizeof(storage));
        SYMBOL_INFO *symbol = (SYMBOL_INFO *)storage.bytes;
        symbol->SizeOfStruct = sizeof(*symbol);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(trace->symbol_process, (DWORD64)address, &displacement,
                        symbol)) {
            size_t length = symbol->NameLen;
            if (length >= capacity) length = capacity - 1u;
            memcpy(buffer, symbol->Name, length);
            buffer[length] = '\0';
            return length;
        }
    }
#else
    (void)trace;
#endif

    int length = snprintf(buffer, capacity, "0x%" PRIxPTR, address);
    if (length < 0) {
        if (capacity != 0) buffer[0] = '\0';
        return 0;
    }
    return (size_t)length < capacity ? (size_t)length : capacity - 1u;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__write_record_locked(
    LorTraceImpl *trace, uint64_t thread_id, LorTraceRecordHeader header,
    const char *name) {
    char auto_name[1024];
    const char *event_name = name;
    size_t event_name_size = header.name_size;
    const char *category = "lor";
    const char *phase = NULL;

    switch ((LorTraceRecordType)header.type) {
    case LOR_TRACE__RECORD_BEGIN:
        phase = "B";
        break;
    case LOR_TRACE__RECORD_END:
        return lor_trace__event_separator(trace) &&
               lor_trace__write_literal(trace, "{\"ph\":\"E\",\"pid\":") &&
               lor_trace__write_u64(trace, trace->process_id) &&
               lor_trace__write_literal(trace, ",\"tid\":") &&
               lor_trace__write_u64(trace, thread_id) &&
               lor_trace__write_literal(trace, ",\"ts\":") &&
               lor_trace__write_timestamp(trace, header.timestamp_ns) &&
               lor_trace__write_char(trace, '}');
    case LOR_TRACE__RECORD_INSTANT:
        phase = "i";
        break;
    case LOR_TRACE__RECORD_COUNTER:
        phase = "C";
        break;
    case LOR_TRACE__RECORD_AUTO_BEGIN:
        phase = "B";
        category = "lor.auto";
        event_name_size = lor_trace__auto_name(trace, (uintptr_t)header.value,
                                               auto_name, sizeof(auto_name));
        event_name = auto_name;
        break;
    default:
        trace->failed = 1;
        return 0;
    }

    if (!lor_trace__event_separator(trace) ||
        !lor_trace__write_literal(trace, "{\"name\":") ||
        !lor_trace__write_json_string(trace, event_name, event_name_size) ||
        !lor_trace__write_literal(trace, ",\"cat\":") ||
        !lor_trace__write_json_string(trace, category, strlen(category)) ||
        !lor_trace__write_literal(trace, ",\"ph\":") ||
        !lor_trace__write_json_string(trace, phase, 1) ||
        !lor_trace__write_literal(trace, ",\"pid\":") ||
        !lor_trace__write_u64(trace, trace->process_id) ||
        !lor_trace__write_literal(trace, ",\"tid\":") ||
        !lor_trace__write_u64(trace, thread_id) ||
        !lor_trace__write_literal(trace, ",\"ts\":") ||
        !lor_trace__write_timestamp(trace, header.timestamp_ns))
        return 0;

    if (header.type == LOR_TRACE__RECORD_INSTANT &&
        !lor_trace__write_literal(trace, ",\"s\":\"t\""))
        return 0;

    if (header.type == LOR_TRACE__RECORD_COUNTER) {
        int64_t counter = 0;
        memcpy(&counter, &header.value, sizeof(counter));
        if (!lor_trace__write_literal(trace, ",\"args\":{\"value\":") ||
            !lor_trace__write_i64(trace, counter) ||
            !lor_trace__write_char(trace, '}'))
            return 0;
    }

    return lor_trace__write_char(trace, '}');
}

static LOR_TRACE__NOINSTRUMENT LorStatus
lor_trace__thread_flush_internal(LorTraceThreadImpl *thread, int flush_stream) {
    LorTraceImpl *trace = thread->trace;
    if (thread->used == 0) {
        if (!flush_stream) return LOR_STATUS_OK;
        lor_trace__lock(trace);
        int ok = !trace->closed && !trace->failed && fflush(trace->file) == 0;
        if (!ok) trace->failed = 1;
        lor_trace__unlock(trace);
        return ok ? LOR_STATUS_OK : LOR_STATUS_SYSTEM_ERROR;
    }

    lor_trace__lock(trace);
    if (trace->closed || trace->failed) {
        trace->failed = 1;
        lor_trace__unlock(trace);
        return LOR_STATUS_SYSTEM_ERROR;
    }

    size_t offset = 0;
    while (offset < thread->used) {
        if (thread->used - offset < sizeof(LorTraceRecordHeader)) {
            trace->failed = 1;
            break;
        }

        LorTraceRecordHeader header;
        memcpy(&header, thread->buffer + offset, sizeof(header));
        offset += sizeof(header);
        if (header.name_size > thread->used - offset) {
            trace->failed = 1;
            break;
        }

        const char *name = (const char *)(thread->buffer + offset);
        if (!lor_trace__write_record_locked(trace, thread->thread_id, header, name))
            break;
        offset += header.name_size;
    }

    if (!trace->failed && flush_stream && fflush(trace->file) != 0)
        trace->failed = 1;

    int ok = !trace->failed;
    if (ok) thread->used = 0;
    lor_trace__unlock(trace);
    return ok ? LOR_STATUS_OK : LOR_STATUS_SYSTEM_ERROR;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__space_available(
    const LorTraceThreadImpl *thread, size_t record_size,
    size_t reserved_ends_after) {
    if (reserved_ends_after >
        (SIZE_MAX - record_size) / sizeof(LorTraceRecordHeader))
        return 0;
    size_t required =
        record_size + reserved_ends_after * sizeof(LorTraceRecordHeader);
    return required <= thread->capacity - thread->used;
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__ensure_space(
    LorTraceThreadImpl *thread, size_t record_size, size_t reserved_ends_after) {
    if (lor_trace__space_available(thread, record_size, reserved_ends_after))
        return 1;
    if (thread->trace->buffer_mode != LOR_TRACE_BUFFER_FLUSH) return 0;
    if (lor_trace__thread_flush_internal(thread, 0) != LOR_STATUS_OK) return 0;
    return lor_trace__space_available(thread, record_size, reserved_ends_after);
}

static LOR_TRACE__NOINSTRUMENT int lor_trace__append(LorTraceThreadImpl *thread,
                                                     LorTraceRecordType type,
                                                     const char *name,
                                                     uint64_t value,
                                                     size_t reserved_ends_after) {
    size_t name_size = name != NULL ? strlen(name) : 0;
    if (name_size > UINT32_MAX ||
        name_size > SIZE_MAX - sizeof(LorTraceRecordHeader))
        return 0;

    size_t record_size = sizeof(LorTraceRecordHeader) + name_size;
    if (!lor_trace__ensure_space(thread, record_size, reserved_ends_after)) return 0;

    LorTraceRecordHeader header = {
        .timestamp_ns = lor_trace__timestamp(thread->trace),
        .value = value,
        .name_size = (uint32_t)name_size,
        .type = (uint8_t)type,
        .reserved = {0, 0, 0},
    };
    memcpy(thread->buffer + thread->used, &header, sizeof(header));
    thread->used += sizeof(header);
    if (name_size != 0) {
        memcpy(thread->buffer + thread->used, name, name_size);
        thread->used += name_size;
    }
    ++thread->recorded_events;
    return 1;
}

static LOR_TRACE__NOINSTRUMENT void lor_trace__begin_value(
    LorTraceThreadImpl *thread, LorTraceRecordType type, const char *name,
    uint64_t value) {
    ++thread->depth;
    if (thread->suppressed_depth != 0) {
        ++thread->suppressed_depth;
        ++thread->dropped_events;
        return;
    }

    if (thread->reserved_ends == SIZE_MAX ||
        !lor_trace__append(thread, type, name, value, thread->reserved_ends + 1u)) {
        thread->suppressed_depth = 1;
        ++thread->dropped_events;
        return;
    }
    ++thread->reserved_ends;
}

static LOR_TRACE__NOINSTRUMENT void lor_trace__end_impl(LorTraceThreadImpl *thread) {
    if (thread->depth == 0) {
        ++thread->dropped_events;
        return;
    }

    --thread->depth;
    if (thread->suppressed_depth != 0) {
        --thread->suppressed_depth;
        ++thread->dropped_events;
        return;
    }

    if (thread->reserved_ends == 0 ||
        !lor_trace__append(thread, LOR_TRACE__RECORD_END, NULL, 0,
                           thread->reserved_ends - 1u)) {
        ++thread->dropped_events;
        if (thread->reserved_ends != 0) --thread->reserved_ends;
        return;
    }
    --thread->reserved_ends;
}

LOR_TRACE__NOINSTRUMENT LorStatus lor_trace_init_file(LorTrace *trace,
                                                      const char *path,
                                                      LorTraceConfig config) {
    if (trace == NULL || trace->impl != NULL || path == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    if (config.buffer_mode != LOR_TRACE_BUFFER_FLUSH &&
        config.buffer_mode != LOR_TRACE_BUFFER_DROP)
        return LOR_STATUS_INVALID_ARGUMENT;

    size_t buffer_size =
        config.buffer_size != 0 ? config.buffer_size : LOR_TRACE_DEFAULT_BUFFER_SIZE;
    if (buffer_size < LOR_TRACE_MIN_BUFFER_SIZE) return LOR_STATUS_INVALID_ARGUMENT;

    LorTraceImpl *impl = (LorTraceImpl *)calloc(1, sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    atomic_flag_clear(&impl->lock);
    impl->buffer_size = buffer_size;
    impl->buffer_mode = config.buffer_mode;
    impl->process_id = lor_trace__process_id();
    impl->next_thread_id = 1;
    impl->first_event = 1;

    if (!lor_trace__clock_init(impl)) {
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }
    impl->origin_ns = lor_trace__now_ns(impl);

    impl->file = lor_trace__open_file(path);
    if (impl->file == NULL) {
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }
    if (!lor_trace__write_literal(impl, "[\n")) {
        (void)fclose(impl->file);
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }

#if defined(_WIN32) && defined(LOR_TRACE_AUTO)
    impl->symbol_process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    impl->symbols_ready = SymInitialize(impl->symbol_process, NULL, TRUE) ? 1 : 0;
#endif

    if (config.process_name != NULL &&
        !lor_trace__write_metadata_locked(impl, "process_name", 0,
                                          config.process_name)) {
#if defined(_WIN32) && defined(LOR_TRACE_AUTO)
        if (impl->symbols_ready) (void)SymCleanup(impl->symbol_process);
#endif
        (void)fclose(impl->file);
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }

    trace->impl = impl;
    return LOR_STATUS_OK;
}

LOR_TRACE__NOINSTRUMENT LorStatus lor_trace_finish(LorTrace *trace) {
    if (trace == NULL || trace->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    LorTraceImpl *impl = (LorTraceImpl *)trace->impl;

    lor_trace__lock(impl);
    if (impl->active_threads != 0) {
        lor_trace__unlock(impl);
        return LOR_STATUS_INVALID_ARGUMENT;
    }

    int ok = !impl->failed;
    if (!lor_trace__write_literal(impl, "\n]\n")) ok = 0;
    if (fflush(impl->file) != 0) ok = 0;
    if (fclose(impl->file) != 0) ok = 0;
    impl->file = NULL;
    impl->closed = 1;
#if defined(_WIN32) && defined(LOR_TRACE_AUTO)
    if (impl->symbols_ready && !SymCleanup(impl->symbol_process)) ok = 0;
#endif
    trace->impl = NULL;
    lor_trace__unlock(impl);
    free(impl);
    return ok ? LOR_STATUS_OK : LOR_STATUS_SYSTEM_ERROR;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_deinit(LorTrace *trace) {
    if (trace == NULL || trace->impl == NULL) return;
    (void)lor_trace_finish(trace);
}

LOR_TRACE__NOINSTRUMENT LorStatus lor_trace_thread_init(LorTraceThread *thread,
                                                        LorTrace *trace,
                                                        const char *name) {
    if (thread == NULL || thread->impl != NULL || trace == NULL ||
        trace->impl == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;

    LorTraceImpl *trace_impl = (LorTraceImpl *)trace->impl;
    LorTraceThreadImpl *impl = (LorTraceThreadImpl *)calloc(1, sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    impl->buffer = (unsigned char *)malloc(trace_impl->buffer_size);
    if (impl->buffer == NULL) {
        free(impl);
        return LOR_STATUS_OUT_OF_MEMORY;
    }
    impl->trace = trace_impl;
    impl->capacity = trace_impl->buffer_size;

    lor_trace__lock(trace_impl);
    int ok = !trace_impl->closed && !trace_impl->failed;
    if (ok) {
        impl->thread_id = trace_impl->next_thread_id++;
        if (name != NULL)
            ok = lor_trace__write_metadata_locked(trace_impl, "thread_name",
                                                  impl->thread_id, name);
        if (ok) ++trace_impl->active_threads;
    }
    lor_trace__unlock(trace_impl);

    if (!ok) {
        free(impl->buffer);
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }

    thread->impl = impl;
    return LOR_STATUS_OK;
}

LOR_TRACE__NOINSTRUMENT LorStatus lor_trace_thread_flush(LorTraceThread *thread) {
    if (thread == NULL || thread->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    return lor_trace__thread_flush_internal((LorTraceThreadImpl *)thread->impl, 1);
}

LOR_TRACE__NOINSTRUMENT LorStatus lor_trace_thread_finish(LorTraceThread *thread) {
    if (thread == NULL || thread->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    LorTraceThreadImpl *impl = (LorTraceThreadImpl *)thread->impl;
    int unbalanced = impl->depth != 0;

    if (lor_trace__auto_thread == thread) {
        lor_trace__auto_thread = NULL;
        lor_trace__auto_enabled = 0;
    }

    impl->depth -= impl->suppressed_depth;
    impl->suppressed_depth = 0;
    while (impl->reserved_ends != 0) {
        if (!lor_trace__append(impl, LOR_TRACE__RECORD_END, NULL, 0,
                               impl->reserved_ends - 1u))
            ++impl->dropped_events;
        --impl->reserved_ends;
        if (impl->depth != 0) --impl->depth;
    }
    impl->depth = 0;

    LorStatus status = lor_trace__thread_flush_internal(impl, 1);
    LorTraceImpl *trace = impl->trace;
    lor_trace__lock(trace);
    if (trace->active_threads != 0) --trace->active_threads;
    lor_trace__unlock(trace);

    free(impl->buffer);
    free(impl);
    thread->impl = NULL;
    if (status != LOR_STATUS_OK) return status;
    return unbalanced ? LOR_STATUS_INVALID_ARGUMENT : LOR_STATUS_OK;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_thread_deinit(LorTraceThread *thread) {
    if (thread == NULL || thread->impl == NULL) return;
    (void)lor_trace_thread_finish(thread);
}

LOR_TRACE__NOINSTRUMENT LorTraceThreadStats
lor_trace_thread_stats(const LorTraceThread *thread) {
    LorTraceThreadStats stats = {0};
    if (thread == NULL || thread->impl == NULL) return stats;
    const LorTraceThreadImpl *impl = (const LorTraceThreadImpl *)thread->impl;
    stats.recorded_events = impl->recorded_events;
    stats.dropped_events = impl->dropped_events;
    stats.open_zones = impl->depth;
    stats.buffered_bytes = impl->used;
    stats.buffer_capacity = impl->capacity;
    return stats;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_begin(LorTraceThread *thread,
                                             const char *name) {
    if (thread == NULL || thread->impl == NULL || name == NULL) return;
    lor_trace__begin_value((LorTraceThreadImpl *)thread->impl,
                           LOR_TRACE__RECORD_BEGIN, name, 0);
}

LOR_TRACE__NOINSTRUMENT void lor_trace_end(LorTraceThread *thread) {
    if (thread == NULL || thread->impl == NULL) return;
    lor_trace__end_impl((LorTraceThreadImpl *)thread->impl);
}

LOR_TRACE__NOINSTRUMENT void lor_trace_instant(LorTraceThread *thread,
                                               const char *name) {
    if (thread == NULL || thread->impl == NULL || name == NULL) return;
    LorTraceThreadImpl *impl = (LorTraceThreadImpl *)thread->impl;
    if (impl->suppressed_depth != 0 ||
        !lor_trace__append(impl, LOR_TRACE__RECORD_INSTANT, name, 0,
                           impl->reserved_ends))
        ++impl->dropped_events;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_counter(LorTraceThread *thread,
                                               const char *name, int64_t value) {
    if (thread == NULL || thread->impl == NULL || name == NULL) return;
    LorTraceThreadImpl *impl = (LorTraceThreadImpl *)thread->impl;
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    if (impl->suppressed_depth != 0 ||
        !lor_trace__append(impl, LOR_TRACE__RECORD_COUNTER, name, bits,
                           impl->reserved_ends))
        ++impl->dropped_events;
}

LOR_TRACE__NOINSTRUMENT LorTraceScope lor_trace_scope_begin(LorTraceThread *thread,
                                                            const char *name) {
    LorTraceScope scope = {NULL};
    if (thread == NULL || thread->impl == NULL || name == NULL) return scope;
    lor_trace_begin(thread, name);
    scope.thread = thread;
    return scope;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_scope_end(LorTraceScope *scope) {
    if (scope == NULL || scope->thread == NULL) return;
    lor_trace_end(scope->thread);
    scope->thread = NULL;
}

LOR_TRACE__NOINSTRUMENT LorStatus lor_trace_auto_bind(LorTraceThread *thread) {
#if LOR_TRACE_AUTO_SUPPORTED
    if (thread == NULL || thread->impl == NULL ||
        (lor_trace__auto_thread != NULL && lor_trace__auto_thread != thread))
        return LOR_STATUS_INVALID_ARGUMENT;
    lor_trace__auto_thread = thread;
    lor_trace__auto_enabled = 1;
    return LOR_STATUS_OK;
#else
    (void)thread;
    return LOR_STATUS_SYSTEM_ERROR;
#endif
}

LOR_TRACE__NOINSTRUMENT void lor_trace_auto_unbind(void) {
    lor_trace__auto_enabled = 0;
    lor_trace__auto_thread = NULL;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_auto_enable(void) {
    if (lor_trace__auto_thread != NULL) lor_trace__auto_enabled = 1;
}

LOR_TRACE__NOINSTRUMENT void lor_trace_auto_disable(void) {
    lor_trace__auto_enabled = 0;
}

LOR_TRACE__NOINSTRUMENT int lor_trace_auto_is_enabled(void) {
    return lor_trace__auto_thread != NULL && lor_trace__auto_enabled;
}

#if LOR_TRACE_AUTO_SUPPORTED
LOR_TRACE__NOINSTRUMENT void __cyg_profile_func_enter(void *function, void *caller) {
    (void)caller;
    if (!lor_trace__auto_enabled || lor_trace__auto_thread == NULL) return;

    lor_trace__auto_enabled = 0;
    lor_trace__begin_value((LorTraceThreadImpl *)lor_trace__auto_thread->impl,
                           LOR_TRACE__RECORD_AUTO_BEGIN, NULL,
                           (uint64_t)(uintptr_t)function);
    lor_trace__auto_enabled = 1;
}

LOR_TRACE__NOINSTRUMENT void __cyg_profile_func_exit(void *function, void *caller) {
    (void)function;
    (void)caller;
    if (!lor_trace__auto_enabled || lor_trace__auto_thread == NULL) return;

    lor_trace__auto_enabled = 0;
    lor_trace__end_impl((LorTraceThreadImpl *)lor_trace__auto_thread->impl);
    lor_trace__auto_enabled = 1;
}
#endif
