# Concurrency

`lor/concurrency.h` provides a portable concurrency layer built on native
Windows threads and POSIX threads. It uses operating-system threads for
predictable scheduling and resource management.

## Features

- Joinable threads
- Mutexes and condition variables
- Monotonic deadlines and sleeping
- Structured task groups with cooperative cancellation
- Buffered and unbuffered typed channels

## Threads

`LorThread` owns one joinable operating-system thread:

```c
static int worker(void *user) {
    int *value = user;
    *value = 42;
    return 7;
}

LorThread thread = LOR_THREAD_INIT;
int value = 0;

lor_thread_start(&thread, worker, &value);

int result;
lor_thread_join(&thread, &result);
```

Joining resets the handle. `lor_thread_deinit` also joins, discards the result,
and resets the handle.

`lor_mutex_init` and `lor_mutex_deinit` manage `LorMutex` lifetimes. `lor_cond_init`
and `lor_cond_deinit` manage `LorCond`. Condition waits may wake spuriously, so
the protected predicate must always be checked in a loop. `lor_cond_signal` and
`lor_cond_broadcast` wake waiting threads.

## Deadlines

Timed operations use absolute `LorDeadline` values:

```c
LorDeadline deadline = lor_deadline_after_ms(250);
LorStatus status =
    lor_cond_wait_until(&condition, &mutex, deadline);
```

Absolute deadlines prevent repeated wakeups from restarting a relative timeout.
`LOR_DEADLINE_INFINITE` selects an untimed wait. `lor_time_now_ms` returns the
current monotonic time.

`lor_sleep_ms` and `lor_sleep_until` provide thread sleeping with relative or
absolute durations.

## Task Groups

`LorTaskGroup` owns all tasks spawned into it:

```c
static void task(LorTaskContext *context, void *user) {
    while (!lor_task_cancelled(context)) {
        // Do bounded work.
    }
}

LorTaskGroup group = LOR_TASK_GROUP_INIT;
lor_task_group_init(&group);
lor_task_group_spawn(&group, task, NULL);

lor_task_group_cancel(&group);
lor_task_group_wait(&group);
lor_task_group_deinit(&group);
```

Each task currently uses one OS thread. This is suitable for a modest number of
coarse tasks.

Cancellation is cooperative. It sets a flag observed through
`lor_task_cancelled`; it cannot forcibly stop a thread or interrupt arbitrary
user code. Channel waits do not implicitly observe a task group's cancellation
flag. A typical channel-based shutdown closes the channels needed to wake
workers, requests cancellation, and then waits for the group.

Waiting prevents further spawning. Deinitialization requests cancellation and
joins all children before releasing the group. The owner must not deinitialize
the group from one of its own child tasks.

## Channels

Channels copy fixed-size values:

```c
LorChannel values = LOR_CHANNEL_INIT;
lor_channel_init(&values, int, 8);

int sent = 10;
lor_channel_send(&values, &sent);

int received;
lor_channel_receive(&values, &received);

lor_channel_close(&values);
lor_channel_deinit(&values);
```

The typed macros infer `sizeof *value_pointer` and the implementation verifies
that it matches the element size selected during initialization. The channel
copies bytes and does not acquire ownership of pointers contained in a value.

A positive capacity creates a FIFO buffer. Capacity zero creates a rendezvous
channel: a send waits until a receiver is present. Timed sends and receives
return `LOR_STATUS_TIMED_OUT`.

`lor_channel_is_closed`, `lor_channel_element_size`, and `lor_channel_capacity`
expose the current state of a channel.

Closing a channel wakes blocked operations:

- sends return `LOR_STATUS_CLOSED`;
- buffered values remain receivable;
- receives return `LOR_STATUS_CLOSED` after the buffer is drained.

Close is idempotent. Before deinitializing, the owner must ensure all active
senders and receivers have returned and no new operation can begin.

See `examples/concurrency.c`.
