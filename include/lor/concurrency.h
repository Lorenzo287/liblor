// SPDX-License-Identifier: MIT

#ifndef LOR_CONCURRENCY_H
#define LOR_CONCURRENCY_H

#include <stddef.h>
#include <stdint.h>

#include "lor/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Absolute deadline used by timed waits.

   Values come from `lor_time_now_ms` or `lor_deadline_after_ms`. */
typedef int64_t LorDeadline;

#define LOR_DEADLINE_INFINITE INT64_MAX

// Returns monotonic milliseconds from an unspecified origin.
LorDeadline lor_time_now_ms(void);

/* Returns a deadline `milliseconds` from now.

   Non-positive durations produce an already-expired deadline. Overflow
   saturates to `LOR_DEADLINE_INFINITE`. */
LorDeadline lor_deadline_after_ms(int64_t milliseconds);

// Sleeps until `deadline`, or returns immediately when it has expired.
void lor_sleep_until(LorDeadline deadline);

// Sleeps for at least `milliseconds` when it is positive.
void lor_sleep_ms(int64_t milliseconds);

typedef int (*LorThreadFn)(void *user);

typedef struct LorThread {
    void *impl;
} LorThread;

#define LOR_THREAD_INIT {NULL}

/* Starts a joinable operating-system thread.

   `thread` must be initialized and not already running. The function and user
   pointer must remain valid until the thread exits. */
LorStatus lor_thread_start(LorThread *thread, LorThreadFn function, void *user);

/* Waits for `thread` to finish and resets it to `LOR_THREAD_INIT`.

   The function result is stored in `result` when it is non-NULL. */
LorStatus lor_thread_join(LorThread *thread, int *result);

// Joins a running thread, discards its result, and resets the handle.
void lor_thread_deinit(LorThread *thread);

typedef struct LorMutex {
    void *impl;
} LorMutex;

#define LOR_MUTEX_INIT {NULL}

LorStatus lor_mutex_init(LorMutex *mutex);
void lor_mutex_deinit(LorMutex *mutex);
LorStatus lor_mutex_lock(LorMutex *mutex);
LorStatus lor_mutex_unlock(LorMutex *mutex);

typedef struct LorCond {
    void *impl;
} LorCond;

#define LOR_COND_INIT {NULL}

LorStatus lor_cond_init(LorCond *condition);
void lor_cond_deinit(LorCond *condition);

/* Atomically releases `mutex`, waits, then reacquires it.

   Spurious wakeups are permitted, so callers must always recheck their
   predicate while holding the mutex. */
LorStatus lor_cond_wait(LorCond *condition, LorMutex *mutex);

// Equivalent to `lor_cond_wait`, with an absolute deadline.
LorStatus lor_cond_wait_until(LorCond *condition, LorMutex *mutex,
                              LorDeadline deadline);

LorStatus lor_cond_signal(LorCond *condition);
LorStatus lor_cond_broadcast(LorCond *condition);

typedef struct LorTaskContext {
    const void *state;
} LorTaskContext;

typedef void (*LorTaskFn)(LorTaskContext *context, void *user);

typedef struct LorTaskGroup {
    void *impl;
} LorTaskGroup;

#define LOR_TASK_GROUP_INIT {NULL}

/* Initializes a structured group of operating-system tasks.

   Each spawned task owns one joinable OS thread. This favors portability and
   predictable behavior over coroutine-scale task counts. */
LorStatus lor_task_group_init(LorTaskGroup *group);

/* Starts one child task owned by `group`.

   Spawn is no longer permitted after waiting or cancellation begins. */
LorStatus lor_task_group_spawn(LorTaskGroup *group, LorTaskFn function, void *user);

/* Requests cooperative cancellation.

   Cancellation does not forcibly terminate threads. Tasks should check
   `lor_task_cancelled`, close shared channels, or otherwise arrange for
   blocking operations to finish. */
void lor_task_group_cancel(LorTaskGroup *group);
int lor_task_group_cancelled(const LorTaskGroup *group);
int lor_task_cancelled(const LorTaskContext *context);

/* Prevents new tasks, then waits for all current children.

   Repeated calls succeed. The group remains valid until deinitialized. */
LorStatus lor_task_group_wait(LorTaskGroup *group);

/* Requests cancellation, joins every child, and resets the group.

   The owning thread must not call this from one of the group's child tasks. */
void lor_task_group_deinit(LorTaskGroup *group);

typedef struct LorChannel {
    void *impl;
} LorChannel;

#define LOR_CHANNEL_INIT {NULL}

/* Initializes a fixed-element-size channel.

   A zero capacity creates an unbuffered rendezvous channel. Positive
   capacities create FIFO buffered channels. */
LorStatus lor_channel_init_raw(LorChannel *channel, size_t element_size,
                               size_t capacity);

/* Closes `channel` and wakes blocked operations.

   Buffered values remain receivable. Sending after close returns
   `LOR_STATUS_CLOSED`; receiving returns it after buffered values are drained. */
LorStatus lor_channel_close(LorChannel *channel);

/* Closes and releases channel storage.

   The caller must first ensure all active operations have returned and no new
   operation can begin concurrently with deinitialization. */
void lor_channel_deinit(LorChannel *channel);

int lor_channel_is_closed(const LorChannel *channel);
size_t lor_channel_element_size(const LorChannel *channel);
size_t lor_channel_capacity(const LorChannel *channel);

LorStatus lor_channel_send_raw(LorChannel *channel, const void *value,
                               size_t value_size);
LorStatus lor_channel_send_until_raw(LorChannel *channel, const void *value,
                                     size_t value_size, LorDeadline deadline);
LorStatus lor_channel_receive_raw(LorChannel *channel, void *value,
                                  size_t value_size);
LorStatus lor_channel_receive_until_raw(LorChannel *channel, void *value,
                                        size_t value_size, LorDeadline deadline);

#define lor_channel_init(channel, type, capacity) \
    lor_channel_init_raw((channel), sizeof(type), (capacity))

#define lor_channel_send(channel, value_pointer) \
    lor_channel_send_raw((channel), (value_pointer), sizeof *(value_pointer))

#define lor_channel_send_until(channel, value_pointer, deadline)                    \
    lor_channel_send_until_raw((channel), (value_pointer), sizeof *(value_pointer), \
                               (deadline))

#define lor_channel_receive(channel, value_pointer) \
    lor_channel_receive_raw((channel), (value_pointer), sizeof *(value_pointer))

#define lor_channel_receive_until(channel, value_pointer, deadline) \
    lor_channel_receive_until_raw((channel), (value_pointer),       \
                                  sizeof *(value_pointer), (deadline))

#ifdef __cplusplus
}
#endif

#endif
