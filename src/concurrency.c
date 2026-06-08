// SPDX-License-Identifier: MIT

#include "lor/concurrency.h"

#include <limits.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <process.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif
#endif

typedef struct LorConcurrencyThreadImpl {
#if defined(_WIN32)
    HANDLE handle;
#else
    pthread_t handle;
#endif
    LorThreadFn function;
    void *user;
    int result;
} LorConcurrencyThreadImpl;

typedef struct LorConcurrencyMutexImpl {
#if defined(_WIN32)
    SRWLOCK lock;
#else
    pthread_mutex_t lock;
#endif
} LorConcurrencyMutexImpl;

typedef struct LorConcurrencyCondImpl {
#if defined(_WIN32)
    CONDITION_VARIABLE condition;
#else
    pthread_cond_t condition;
#endif
} LorConcurrencyCondImpl;

typedef struct LorConcurrencyTaskGroupImpl LorConcurrencyTaskGroupImpl;

typedef struct LorConcurrencyTaskStart {
    LorConcurrencyTaskGroupImpl *group;
    LorTaskFn function;
    void *user;
} LorConcurrencyTaskStart;

struct LorConcurrencyTaskGroupImpl {
    LorThread *threads;
    size_t count;
    size_t capacity;
    atomic_int cancelled;
    int accepting;
    int waited;
};

typedef struct LorConcurrencyChannelImpl {
    LorMutex mutex;
    LorCond can_send;
    LorCond can_receive;
    unsigned char *buffer;
    size_t element_size;
    size_t capacity;
    size_t count;
    size_t first;
    size_t waiting_receivers;
    int slot_full;
    int closed;
} LorConcurrencyChannelImpl;

#if defined(__linux__) && !defined(CLOCK_MONOTONIC)
extern int clock_gettime(int clock_id, struct timespec *time_value);
#define LOR_CONCURRENCY__CLOCK_MONOTONIC 1
#elif defined(CLOCK_MONOTONIC)
#define LOR_CONCURRENCY__CLOCK_MONOTONIC CLOCK_MONOTONIC
#endif

static LorStatus lor_concurrency__status_from_wait_error(int error) {
#if defined(_WIN32)
    if (error == ERROR_TIMEOUT) return LOR_STATUS_TIMED_OUT;
#else
    if (error == ETIMEDOUT) return LOR_STATUS_TIMED_OUT;
#endif
    return LOR_STATUS_SYSTEM_ERROR;
}

LorDeadline lor_time_now_ms(void) {
#if defined(_WIN32)
    ULONGLONG value = GetTickCount64();
    return value > (ULONGLONG)INT64_MAX ? INT64_MAX : (LorDeadline)value;
#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase = {0, 0};
    if (timebase.denom == 0) (void)mach_timebase_info(&timebase);

    uint64_t ticks = mach_absolute_time();
    uint64_t nanoseconds =
        timebase.denom == 0
            ? ticks
            : ticks / timebase.denom * timebase.numer +
                  ticks % timebase.denom * timebase.numer / timebase.denom;
    uint64_t milliseconds = nanoseconds / UINT64_C(1000000);
    return milliseconds > (uint64_t)INT64_MAX ? INT64_MAX
                                              : (LorDeadline)milliseconds;
#elif defined(LOR_CONCURRENCY__CLOCK_MONOTONIC)
    struct timespec value;
    if (clock_gettime(LOR_CONCURRENCY__CLOCK_MONOTONIC, &value) != 0) return 0;
    if (value.tv_sec > INT64_MAX / 1000) return INT64_MAX;
    return (LorDeadline)value.tv_sec * 1000 + (LorDeadline)(value.tv_nsec / 1000000);
#else
    struct timespec value;
    if (timespec_get(&value, TIME_UTC) != TIME_UTC) return 0;
    if (value.tv_sec > INT64_MAX / 1000) return INT64_MAX;
    return (LorDeadline)value.tv_sec * 1000 + (LorDeadline)(value.tv_nsec / 1000000);
#endif
}

LorDeadline lor_deadline_after_ms(int64_t milliseconds) {
    LorDeadline now = lor_time_now_ms();
    if (milliseconds <= 0) return now;
    if (now >= LOR_DEADLINE_INFINITE - milliseconds) return LOR_DEADLINE_INFINITE;
    return now + milliseconds;
}

#if !defined(_WIN32)
static struct timespec lor_concurrency__realtime_after_ms(int64_t milliseconds) {
    struct timespec value = {0, 0};
    (void)timespec_get(&value, TIME_UTC);

    value.tv_sec += (time_t)(milliseconds / 1000);
    value.tv_nsec += (long)(milliseconds % 1000) * 1000000L;
    if (value.tv_nsec >= 1000000000L) {
        value.tv_sec += 1;
        value.tv_nsec -= 1000000000L;
    }
    return value;
}
#endif

void lor_sleep_until(LorDeadline deadline) {
#if defined(_WIN32)
    if (deadline == LOR_DEADLINE_INFINITE) {
        Sleep(INFINITE);
        return;
    }
    for (;;) {
        LorDeadline now = lor_time_now_ms();
        if (now >= deadline) return;
        uint64_t remaining = (uint64_t)(deadline - now);
        DWORD duration =
            remaining >= (uint64_t)INFINITE ? INFINITE - 1u : (DWORD)remaining;
        Sleep(duration);
    }
#else
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    if (pthread_mutex_init(&mutex, NULL) != 0) return;
    if (pthread_cond_init(&condition, NULL) != 0) {
        (void)pthread_mutex_destroy(&mutex);
        return;
    }

    (void)pthread_mutex_lock(&mutex);
    for (;;) {
        if (deadline == LOR_DEADLINE_INFINITE) {
            if (pthread_cond_wait(&condition, &mutex) != 0) break;
            continue;
        }
        LorDeadline now = lor_time_now_ms();
        if (now >= deadline) break;
        struct timespec wake = lor_concurrency__realtime_after_ms(deadline - now);
        int result = pthread_cond_timedwait(&condition, &mutex, &wake);
        if (result != 0 && result != ETIMEDOUT) break;
    }
    (void)pthread_mutex_unlock(&mutex);
    (void)pthread_cond_destroy(&condition);
    (void)pthread_mutex_destroy(&mutex);
#endif
}

void lor_sleep_ms(int64_t milliseconds) {
    if (milliseconds <= 0) return;
    lor_sleep_until(lor_deadline_after_ms(milliseconds));
}

#if defined(_WIN32)
static unsigned __stdcall lor_concurrency__thread_entry(void *argument) {
#else
static void *lor_concurrency__thread_entry(void *argument) {
#endif
    LorConcurrencyThreadImpl *impl = (LorConcurrencyThreadImpl *)argument;
    impl->result = impl->function(impl->user);
#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}

LorStatus lor_thread_start(LorThread *thread, LorThreadFn function, void *user) {
    if (thread == NULL || function == NULL || thread->impl != NULL)
        return LOR_STATUS_INVALID_ARGUMENT;

    LorConcurrencyThreadImpl *impl =
        (LorConcurrencyThreadImpl *)calloc(1, sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    impl->function = function;
    impl->user = user;

#if defined(_WIN32)
    uintptr_t handle =
        _beginthreadex(NULL, 0, lor_concurrency__thread_entry, impl, 0, NULL);
    if (handle == 0) {
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }
    impl->handle = (HANDLE)handle;
#else
    if (pthread_create(&impl->handle, NULL, lor_concurrency__thread_entry, impl) !=
        0) {
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }
#endif

    thread->impl = impl;
    return LOR_STATUS_OK;
}

LorStatus lor_thread_join(LorThread *thread, int *result) {
    if (thread == NULL || thread->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;

    LorConcurrencyThreadImpl *impl = (LorConcurrencyThreadImpl *)thread->impl;
#if defined(_WIN32)
    if (WaitForSingleObject(impl->handle, INFINITE) != WAIT_OBJECT_0)
        return LOR_STATUS_SYSTEM_ERROR;
    LorStatus status =
        CloseHandle(impl->handle) ? LOR_STATUS_OK : LOR_STATUS_SYSTEM_ERROR;
#else
    if (pthread_join(impl->handle, NULL) != 0) return LOR_STATUS_SYSTEM_ERROR;
#endif

    if (result != NULL) *result = impl->result;
    free(impl);
    thread->impl = NULL;
#if defined(_WIN32)
    return status;
#else
    return LOR_STATUS_OK;
#endif
}

void lor_thread_deinit(LorThread *thread) {
    if (thread == NULL || thread->impl == NULL) return;
    (void)lor_thread_join(thread, NULL);
}

LorStatus lor_mutex_init(LorMutex *mutex) {
    if (mutex == NULL || mutex->impl != NULL) return LOR_STATUS_INVALID_ARGUMENT;

    LorConcurrencyMutexImpl *impl = (LorConcurrencyMutexImpl *)malloc(sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;

#if defined(_WIN32)
    InitializeSRWLock(&impl->lock);
#else
    if (pthread_mutex_init(&impl->lock, NULL) != 0) {
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }
#endif

    mutex->impl = impl;
    return LOR_STATUS_OK;
}

void lor_mutex_deinit(LorMutex *mutex) {
    if (mutex == NULL || mutex->impl == NULL) return;
    LorConcurrencyMutexImpl *impl = (LorConcurrencyMutexImpl *)mutex->impl;
#if !defined(_WIN32)
    (void)pthread_mutex_destroy(&impl->lock);
#endif
    free(impl);
    mutex->impl = NULL;
}

LorStatus lor_mutex_lock(LorMutex *mutex) {
    if (mutex == NULL || mutex->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyMutexImpl *impl = (LorConcurrencyMutexImpl *)mutex->impl;
#if defined(_WIN32)
    AcquireSRWLockExclusive(&impl->lock);
    return LOR_STATUS_OK;
#else
    return pthread_mutex_lock(&impl->lock) == 0 ? LOR_STATUS_OK
                                                : LOR_STATUS_SYSTEM_ERROR;
#endif
}

LorStatus lor_mutex_unlock(LorMutex *mutex) {
    if (mutex == NULL || mutex->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyMutexImpl *impl = (LorConcurrencyMutexImpl *)mutex->impl;
#if defined(_WIN32)
    ReleaseSRWLockExclusive(&impl->lock);
    return LOR_STATUS_OK;
#else
    return pthread_mutex_unlock(&impl->lock) == 0 ? LOR_STATUS_OK
                                                  : LOR_STATUS_SYSTEM_ERROR;
#endif
}

LorStatus lor_cond_init(LorCond *condition) {
    if (condition == NULL || condition->impl != NULL)
        return LOR_STATUS_INVALID_ARGUMENT;

    LorConcurrencyCondImpl *impl = (LorConcurrencyCondImpl *)malloc(sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;

#if defined(_WIN32)
    InitializeConditionVariable(&impl->condition);
#else
    if (pthread_cond_init(&impl->condition, NULL) != 0) {
        free(impl);
        return LOR_STATUS_SYSTEM_ERROR;
    }
#endif

    condition->impl = impl;
    return LOR_STATUS_OK;
}

void lor_cond_deinit(LorCond *condition) {
    if (condition == NULL || condition->impl == NULL) return;
    LorConcurrencyCondImpl *impl = (LorConcurrencyCondImpl *)condition->impl;
#if !defined(_WIN32)
    (void)pthread_cond_destroy(&impl->condition);
#endif
    free(impl);
    condition->impl = NULL;
}

LorStatus lor_cond_wait_until(LorCond *condition, LorMutex *mutex,
                              LorDeadline deadline) {
    if (condition == NULL || condition->impl == NULL || mutex == NULL ||
        mutex->impl == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;

    LorConcurrencyCondImpl *cond_impl = (LorConcurrencyCondImpl *)condition->impl;
    LorConcurrencyMutexImpl *mutex_impl = (LorConcurrencyMutexImpl *)mutex->impl;

#if defined(_WIN32)
    for (;;) {
        DWORD timeout = INFINITE;
        if (deadline == LOR_DEADLINE_INFINITE) {
            if (SleepConditionVariableSRW(&cond_impl->condition, &mutex_impl->lock,
                                          timeout, 0))
                return LOR_STATUS_OK;
            return lor_concurrency__status_from_wait_error((int)GetLastError());
        }

        LorDeadline now = lor_time_now_ms();
        if (now >= deadline)
            timeout = 0;
        else {
            uint64_t remaining = (uint64_t)(deadline - now);
            timeout =
                remaining >= (uint64_t)INFINITE ? INFINITE - 1u : (DWORD)remaining;
        }

        if (SleepConditionVariableSRW(&cond_impl->condition, &mutex_impl->lock,
                                      timeout, 0))
            return LOR_STATUS_OK;
        int error = (int)GetLastError();
        if (error != ERROR_TIMEOUT || lor_time_now_ms() >= deadline)
            return lor_concurrency__status_from_wait_error(error);
    }
#else
    int result;
    if (deadline == LOR_DEADLINE_INFINITE) {
        result = pthread_cond_wait(&cond_impl->condition, &mutex_impl->lock);
    } else {
        LorDeadline now = lor_time_now_ms();
        if (now >= deadline) return LOR_STATUS_TIMED_OUT;
        struct timespec wake = lor_concurrency__realtime_after_ms(deadline - now);
        result =
            pthread_cond_timedwait(&cond_impl->condition, &mutex_impl->lock, &wake);
    }
    return result == 0 ? LOR_STATUS_OK
                       : lor_concurrency__status_from_wait_error(result);
#endif
}

LorStatus lor_cond_wait(LorCond *condition, LorMutex *mutex) {
    return lor_cond_wait_until(condition, mutex, LOR_DEADLINE_INFINITE);
}

LorStatus lor_cond_signal(LorCond *condition) {
    if (condition == NULL || condition->impl == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyCondImpl *impl = (LorConcurrencyCondImpl *)condition->impl;
#if defined(_WIN32)
    WakeConditionVariable(&impl->condition);
    return LOR_STATUS_OK;
#else
    return pthread_cond_signal(&impl->condition) == 0 ? LOR_STATUS_OK
                                                      : LOR_STATUS_SYSTEM_ERROR;
#endif
}

LorStatus lor_cond_broadcast(LorCond *condition) {
    if (condition == NULL || condition->impl == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyCondImpl *impl = (LorConcurrencyCondImpl *)condition->impl;
#if defined(_WIN32)
    WakeAllConditionVariable(&impl->condition);
    return LOR_STATUS_OK;
#else
    return pthread_cond_broadcast(&impl->condition) == 0 ? LOR_STATUS_OK
                                                         : LOR_STATUS_SYSTEM_ERROR;
#endif
}

static int lor_concurrency__task_entry(void *argument) {
    LorConcurrencyTaskStart *start = (LorConcurrencyTaskStart *)argument;
    LorTaskContext context = {.state = start->group};
    LorTaskFn function = start->function;
    void *user = start->user;
    free(start);
    function(&context, user);
    return 0;
}

LorStatus lor_task_group_init(LorTaskGroup *group) {
    if (group == NULL || group->impl != NULL) return LOR_STATUS_INVALID_ARGUMENT;

    LorConcurrencyTaskGroupImpl *impl =
        (LorConcurrencyTaskGroupImpl *)calloc(1, sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    atomic_init(&impl->cancelled, 0);
    impl->accepting = 1;
    group->impl = impl;
    return LOR_STATUS_OK;
}

static LorStatus lor_concurrency__task_group_reserve(
    LorConcurrencyTaskGroupImpl *impl, size_t capacity) {
    if (capacity <= impl->capacity) return LOR_STATUS_OK;
    if (capacity > SIZE_MAX / sizeof(*impl->threads)) return LOR_STATUS_OVERFLOW;

    size_t grown = impl->capacity == 0 ? 4 : impl->capacity;
    while (grown < capacity) {
        if (grown > SIZE_MAX / 2) {
            grown = capacity;
            break;
        }
        grown *= 2;
    }

    LorThread *threads =
        (LorThread *)realloc(impl->threads, grown * sizeof(*threads));
    if (threads == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    impl->threads = threads;
    impl->capacity = grown;
    return LOR_STATUS_OK;
}

LorStatus lor_task_group_spawn(LorTaskGroup *group, LorTaskFn function, void *user) {
    if (group == NULL || group->impl == NULL || function == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyTaskGroupImpl *impl = (LorConcurrencyTaskGroupImpl *)group->impl;
    if (!impl->accepting || atomic_load(&impl->cancelled)) return LOR_STATUS_CLOSED;

    LorStatus status = lor_concurrency__task_group_reserve(impl, impl->count + 1);
    if (status != LOR_STATUS_OK) return status;

    LorConcurrencyTaskStart *start =
        (LorConcurrencyTaskStart *)malloc(sizeof(*start));
    if (start == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    start->group = impl;
    start->function = function;
    start->user = user;

    LorThread thread = LOR_THREAD_INIT;
    status = lor_thread_start(&thread, lor_concurrency__task_entry, start);
    if (status != LOR_STATUS_OK) {
        free(start);
        return status;
    }

    impl->threads[impl->count++] = thread;
    return LOR_STATUS_OK;
}

void lor_task_group_cancel(LorTaskGroup *group) {
    if (group == NULL || group->impl == NULL) return;
    LorConcurrencyTaskGroupImpl *impl = (LorConcurrencyTaskGroupImpl *)group->impl;
    impl->accepting = 0;
    atomic_store(&impl->cancelled, 1);
}

int lor_task_group_cancelled(const LorTaskGroup *group) {
    if (group == NULL || group->impl == NULL) return 1;
    const LorConcurrencyTaskGroupImpl *impl =
        (const LorConcurrencyTaskGroupImpl *)group->impl;
    return atomic_load(&impl->cancelled) != 0;
}

int lor_task_cancelled(const LorTaskContext *context) {
    if (context == NULL || context->state == NULL) return 1;
    const LorConcurrencyTaskGroupImpl *impl =
        (const LorConcurrencyTaskGroupImpl *)context->state;
    return atomic_load(&impl->cancelled) != 0;
}

LorStatus lor_task_group_wait(LorTaskGroup *group) {
    if (group == NULL || group->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyTaskGroupImpl *impl = (LorConcurrencyTaskGroupImpl *)group->impl;
    impl->accepting = 0;
    if (impl->waited) return LOR_STATUS_OK;

    LorStatus result = LOR_STATUS_OK;
    for (size_t i = 0; i < impl->count; ++i) {
        if (impl->threads[i].impl == NULL) continue;
        LorStatus status = lor_thread_join(&impl->threads[i], NULL);
        if (result == LOR_STATUS_OK && status != LOR_STATUS_OK) result = status;
    }
    impl->waited = 1;
    for (size_t i = 0; i < impl->count; ++i) {
        if (impl->threads[i].impl != NULL) {
            impl->waited = 0;
            break;
        }
    }
    return result;
}

void lor_task_group_deinit(LorTaskGroup *group) {
    if (group == NULL || group->impl == NULL) return;
    LorConcurrencyTaskGroupImpl *impl = (LorConcurrencyTaskGroupImpl *)group->impl;
    lor_task_group_cancel(group);
    (void)lor_task_group_wait(group);
    if (!impl->waited) return;
    free(impl->threads);
    free(impl);
    group->impl = NULL;
}

LorStatus lor_channel_init_raw(LorChannel *channel, size_t element_size,
                               size_t capacity) {
    if (channel == NULL || channel->impl != NULL || element_size == 0)
        return LOR_STATUS_INVALID_ARGUMENT;
    if (capacity > 0 && element_size > SIZE_MAX / capacity)
        return LOR_STATUS_OVERFLOW;

    LorConcurrencyChannelImpl *impl =
        (LorConcurrencyChannelImpl *)calloc(1, sizeof(*impl));
    if (impl == NULL) return LOR_STATUS_OUT_OF_MEMORY;
    impl->mutex = (LorMutex)LOR_MUTEX_INIT;
    impl->can_send = (LorCond)LOR_COND_INIT;
    impl->can_receive = (LorCond)LOR_COND_INIT;
    impl->element_size = element_size;
    impl->capacity = capacity;

    size_t storage_count = capacity == 0 ? 1 : capacity;
    impl->buffer = (unsigned char *)malloc(storage_count * element_size);
    if (impl->buffer == NULL) {
        free(impl);
        return LOR_STATUS_OUT_OF_MEMORY;
    }

    LorStatus status = lor_mutex_init(&impl->mutex);
    if (status == LOR_STATUS_OK) status = lor_cond_init(&impl->can_send);
    if (status == LOR_STATUS_OK) status = lor_cond_init(&impl->can_receive);
    if (status != LOR_STATUS_OK) {
        lor_cond_deinit(&impl->can_receive);
        lor_cond_deinit(&impl->can_send);
        lor_mutex_deinit(&impl->mutex);
        free(impl->buffer);
        free(impl);
        return status;
    }

    channel->impl = impl;
    return LOR_STATUS_OK;
}

LorStatus lor_channel_close(LorChannel *channel) {
    if (channel == NULL || channel->impl == NULL) return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyChannelImpl *impl = (LorConcurrencyChannelImpl *)channel->impl;
    LorStatus status = lor_mutex_lock(&impl->mutex);
    if (status != LOR_STATUS_OK) return status;
    impl->closed = 1;
    LorStatus send_status = lor_cond_broadcast(&impl->can_send);
    LorStatus receive_status = lor_cond_broadcast(&impl->can_receive);
    LorStatus unlock_status = lor_mutex_unlock(&impl->mutex);
    if (send_status != LOR_STATUS_OK) return send_status;
    if (receive_status != LOR_STATUS_OK) return receive_status;
    return unlock_status;
}

void lor_channel_deinit(LorChannel *channel) {
    if (channel == NULL || channel->impl == NULL) return;
    LorConcurrencyChannelImpl *impl = (LorConcurrencyChannelImpl *)channel->impl;
    (void)lor_channel_close(channel);
    lor_cond_deinit(&impl->can_receive);
    lor_cond_deinit(&impl->can_send);
    lor_mutex_deinit(&impl->mutex);
    free(impl->buffer);
    free(impl);
    channel->impl = NULL;
}

int lor_channel_is_closed(const LorChannel *channel) {
    if (channel == NULL || channel->impl == NULL) return 1;
    LorConcurrencyChannelImpl *impl = (LorConcurrencyChannelImpl *)channel->impl;
    if (lor_mutex_lock(&impl->mutex) != LOR_STATUS_OK) return 1;
    int closed = impl->closed;
    (void)lor_mutex_unlock(&impl->mutex);
    return closed;
}

size_t lor_channel_element_size(const LorChannel *channel) {
    if (channel == NULL || channel->impl == NULL) return 0;
    const LorConcurrencyChannelImpl *impl =
        (const LorConcurrencyChannelImpl *)channel->impl;
    return impl->element_size;
}

size_t lor_channel_capacity(const LorChannel *channel) {
    if (channel == NULL || channel->impl == NULL) return 0;
    const LorConcurrencyChannelImpl *impl =
        (const LorConcurrencyChannelImpl *)channel->impl;
    return impl->capacity;
}

static LorStatus lor_concurrency__channel_wait(LorCond *condition, LorMutex *mutex,
                                               LorDeadline deadline) {
    return deadline == LOR_DEADLINE_INFINITE
               ? lor_cond_wait(condition, mutex)
               : lor_cond_wait_until(condition, mutex, deadline);
}

LorStatus lor_channel_send_until_raw(LorChannel *channel, const void *value,
                                     size_t value_size, LorDeadline deadline) {
    if (channel == NULL || channel->impl == NULL || value == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyChannelImpl *impl = (LorConcurrencyChannelImpl *)channel->impl;
    if (value_size != impl->element_size) return LOR_STATUS_INVALID_ARGUMENT;

    LorStatus status = lor_mutex_lock(&impl->mutex);
    if (status != LOR_STATUS_OK) return status;

    if (impl->capacity == 0) {
        while (!impl->closed && (impl->waiting_receivers == 0 || impl->slot_full)) {
            status = lor_concurrency__channel_wait(&impl->can_send, &impl->mutex,
                                                   deadline);
            if (status != LOR_STATUS_OK) {
                (void)lor_mutex_unlock(&impl->mutex);
                return status;
            }
        }
        if (impl->closed) {
            (void)lor_mutex_unlock(&impl->mutex);
            return LOR_STATUS_CLOSED;
        }
        memcpy(impl->buffer, value, impl->element_size);
        impl->slot_full = 1;
        (void)lor_cond_signal(&impl->can_receive);
        return lor_mutex_unlock(&impl->mutex);
    }

    while (!impl->closed && impl->count == impl->capacity) {
        status =
            lor_concurrency__channel_wait(&impl->can_send, &impl->mutex, deadline);
        if (status != LOR_STATUS_OK) {
            (void)lor_mutex_unlock(&impl->mutex);
            return status;
        }
    }
    if (impl->closed) {
        (void)lor_mutex_unlock(&impl->mutex);
        return LOR_STATUS_CLOSED;
    }

    size_t position = (impl->first + impl->count) % impl->capacity;
    memcpy(impl->buffer + position * impl->element_size, value, impl->element_size);
    ++impl->count;
    (void)lor_cond_signal(&impl->can_receive);
    return lor_mutex_unlock(&impl->mutex);
}

LorStatus lor_channel_send_raw(LorChannel *channel, const void *value,
                               size_t value_size) {
    return lor_channel_send_until_raw(channel, value, value_size,
                                      LOR_DEADLINE_INFINITE);
}

LorStatus lor_channel_receive_until_raw(LorChannel *channel, void *value,
                                        size_t value_size, LorDeadline deadline) {
    if (channel == NULL || channel->impl == NULL || value == NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    LorConcurrencyChannelImpl *impl = (LorConcurrencyChannelImpl *)channel->impl;
    if (value_size != impl->element_size) return LOR_STATUS_INVALID_ARGUMENT;

    LorStatus status = lor_mutex_lock(&impl->mutex);
    if (status != LOR_STATUS_OK) return status;

    if (impl->capacity == 0) {
        ++impl->waiting_receivers;
        (void)lor_cond_signal(&impl->can_send);
        while (!impl->slot_full && !impl->closed) {
            status = lor_concurrency__channel_wait(&impl->can_receive, &impl->mutex,
                                                   deadline);
            if (status != LOR_STATUS_OK) {
                --impl->waiting_receivers;
                (void)lor_mutex_unlock(&impl->mutex);
                return status;
            }
        }

        --impl->waiting_receivers;
        if (!impl->slot_full) {
            (void)lor_mutex_unlock(&impl->mutex);
            return LOR_STATUS_CLOSED;
        }
        memcpy(value, impl->buffer, impl->element_size);
        impl->slot_full = 0;
        (void)lor_cond_signal(&impl->can_send);
        return lor_mutex_unlock(&impl->mutex);
    }

    while (impl->count == 0 && !impl->closed) {
        status = lor_concurrency__channel_wait(&impl->can_receive, &impl->mutex,
                                               deadline);
        if (status != LOR_STATUS_OK) {
            (void)lor_mutex_unlock(&impl->mutex);
            return status;
        }
    }
    if (impl->count == 0) {
        (void)lor_mutex_unlock(&impl->mutex);
        return LOR_STATUS_CLOSED;
    }

    memcpy(value, impl->buffer + impl->first * impl->element_size,
           impl->element_size);
    impl->first = (impl->first + 1) % impl->capacity;
    --impl->count;
    (void)lor_cond_signal(&impl->can_send);
    return lor_mutex_unlock(&impl->mutex);
}

LorStatus lor_channel_receive_raw(LorChannel *channel, void *value,
                                  size_t value_size) {
    return lor_channel_receive_until_raw(channel, value, value_size,
                                         LOR_DEADLINE_INFINITE);
}
