// SPDX-License-Identifier: MIT

#ifndef LOR_TRACE_H
#define LOR_TRACE_H

#include <stddef.h>
#include <stdint.h>

#include "lor/features.h"
#include "lor/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOR_TRACE_DEFAULT_BUFFER_SIZE ((size_t)1024u * 1024u)
#define LOR_TRACE_MIN_BUFFER_SIZE ((size_t)1024u)

typedef enum LorTraceBufferMode {
    // Flush the thread buffer to the shared output file when it becomes full.
    LOR_TRACE_BUFFER_FLUSH = 0,
    // Drop new events when the thread buffer becomes full.
    LOR_TRACE_BUFFER_DROP = 1
} LorTraceBufferMode;

typedef struct LorTraceConfig {
    // Per-thread event-buffer size. Zero uses LOR_TRACE_DEFAULT_BUFFER_SIZE.
    size_t buffer_size;
    // Determines whether a full buffer is flushed or new events are dropped.
    LorTraceBufferMode buffer_mode;
    // Optional process name copied into trace metadata during initialization.
    const char *process_name;
} LorTraceConfig;

#define LOR_TRACE_CONFIG_INIT {0u, LOR_TRACE_BUFFER_FLUSH, NULL}

typedef struct LorTrace {
    void *impl;
} LorTrace;

#define LOR_TRACE_INIT {NULL}

typedef struct LorTraceThread {
    void *impl;
} LorTraceThread;

#define LOR_TRACE_THREAD_INIT {NULL}

typedef struct LorTraceThreadStats {
    uint64_t recorded_events;
    uint64_t dropped_events;
    size_t open_zones;
    size_t buffered_bytes;
    size_t buffer_capacity;
} LorTraceThreadStats;

/* Opens a Chrome Trace Event JSON file.

   `trace` must be initialized and remain at a stable address until finished.
   The output becomes complete JSON after `lor_trace_finish`. */
LorStatus lor_trace_init_file(LorTrace *trace, const char *path,
                              LorTraceConfig config);

/* Completes and closes the output file, then resets `trace`.

   Every associated `LorTraceThread` must be finished first. */
LorStatus lor_trace_finish(LorTrace *trace);

// Discards the status from `lor_trace_finish`.
void lor_trace_deinit(LorTrace *trace);

/* Initializes a recorder owned by the calling thread.

   Event recording performs no allocation after this call. `name` is copied
   immediately into trace metadata and may be NULL. */
LorStatus lor_trace_thread_init(LorTraceThread *thread, LorTrace *trace,
                                const char *name);

/* Flushes pending events, releases the buffer, and resets `thread`.

   Unclosed zones are closed at the final timestamp so the output remains
   structurally valid, and `LOR_STATUS_INVALID_ARGUMENT` is returned after a
   successful flush. */
LorStatus lor_trace_thread_finish(LorTraceThread *thread);

// Discards the status from `lor_trace_thread_finish`.
void lor_trace_thread_deinit(LorTraceThread *thread);

// Writes pending events from this thread to the shared output file.
LorStatus lor_trace_thread_flush(LorTraceThread *thread);

LorTraceThreadStats lor_trace_thread_stats(const LorTraceThread *thread);

/* Records nested manual zones.

   Names are copied into the thread buffer. Calls on separate thread objects
   may run concurrently; one thread object must only be used by its owner.
   Invalid arguments are ignored. */
void lor_trace_begin(LorTraceThread *thread, const char *name);
void lor_trace_end(LorTraceThread *thread);

// Records an instantaneous event on the current thread track.
void lor_trace_instant(LorTraceThread *thread, const char *name);

// Records a named signed-integer counter value.
void lor_trace_counter(LorTraceThread *thread, const char *name, int64_t value);

typedef struct LorTraceScope {
    LorTraceThread *thread;
} LorTraceScope;

// Portable function form used by the optional cleanup-based scope macro.
LorTraceScope lor_trace_scope_begin(LorTraceThread *thread, const char *name);
void lor_trace_scope_end(LorTraceScope *scope);

/* GCC and Clang automatic function instrumentation.

   Compile both this module's implementation and the instrumented program with
   `LOR_TRACE_AUTO`, then compile the program with `-finstrument-functions`.
   Windows symbolization uses DbgHelp and matching PDB files. Other platforms
   currently write hexadecimal function addresses.

   Bind one initialized trace thread per instrumented operating-system thread.
   Binding is thread-local and records no global default session. */
#if defined(LOR_TRACE_AUTO) && (defined(__GNUC__) || defined(__clang__)) && \
    !defined(_MSC_VER)
#define LOR_TRACE_AUTO_SUPPORTED 1
#elif defined(LOR_TRACE_AUTO) && defined(__clang__)
#define LOR_TRACE_AUTO_SUPPORTED 1
#else
#define LOR_TRACE_AUTO_SUPPORTED 0
#endif

LorStatus lor_trace_auto_bind(LorTraceThread *thread);
void lor_trace_auto_unbind(void);
void lor_trace_auto_enable(void);
void lor_trace_auto_disable(void);
int lor_trace_auto_is_enabled(void);

#if LOR_TRACE_AUTO_SUPPORTED
void __cyg_profile_func_enter(void *function, void *caller);
void __cyg_profile_func_exit(void *function, void *caller);
#endif

#ifdef __cplusplus
}
#endif

#if defined(LOR_TRACE_DISABLED)
#define LOR_TRACE_BEGIN(thread, name) ((void)0)
#define LOR_TRACE_END(thread) ((void)0)
#define LOR_TRACE_INSTANT(thread, name) ((void)0)
#define LOR_TRACE_COUNTER(thread, name, value) ((void)0)
#define LOR_TRACE_SCOPE(thread, name) ((void)0)
#define LOR_TRACE_FUNCTION(thread) ((void)0)
#define LOR_TRACE_SCOPE_SUPPORTED 0
#else
#define LOR_TRACE_BEGIN(thread, name) lor_trace_begin((thread), (name))
#define LOR_TRACE_END(thread) lor_trace_end((thread))
#define LOR_TRACE_INSTANT(thread, name) lor_trace_instant((thread), (name))
#define LOR_TRACE_COUNTER(thread, name, value) \
    lor_trace_counter((thread), (name), (value))

#if LOR_HAS_CLEANUP_ATTRIBUTE
#define LOR_TRACE_SCOPE_SUPPORTED 1
#define LOR_TRACE__JOIN_INNER(a, b) a##b
#define LOR_TRACE__JOIN(a, b) LOR_TRACE__JOIN_INNER(a, b)
#define LOR_TRACE_SCOPE(thread, name)                                   \
    LorTraceScope __attribute__((cleanup(lor_trace_scope_end), unused)) \
    LOR_TRACE__JOIN(lor_trace_scope_, __LINE__) =                       \
        lor_trace_scope_begin((thread), (name))
#define LOR_TRACE_FUNCTION(thread) LOR_TRACE_SCOPE((thread), __func__)
#else
#define LOR_TRACE_SCOPE_SUPPORTED 0
#endif
#endif

#endif
