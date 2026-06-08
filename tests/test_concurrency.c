// SPDX-License-Identifier: MIT

#include "lor/concurrency.h"

#include <stdatomic.h>
#include <stdio.h>

#include "lor_test.h"

typedef struct CounterState {
    LorMutex mutex;
    int value;
} CounterState;

static int return_value_thread(void *user) {
    int *value = (int *)user;
    *value = 42;
    return 7;
}

static int increment_thread(void *user) {
    CounterState *state = (CounterState *)user;
    for (int i = 0; i < 5000; ++i) {
        if (lor_mutex_lock(&state->mutex) != LOR_STATUS_OK) return 1;
        ++state->value;
        if (lor_mutex_unlock(&state->mutex) != LOR_STATUS_OK) return 1;
    }
    return 0;
}

static int test_threads_and_mutex(void) {
    int value = 0;
    LorThread thread = LOR_THREAD_INIT;
    CHECK(lor_thread_start(&thread, return_value_thread, &value) == LOR_STATUS_OK);
    int result = 0;
    CHECK(lor_thread_join(&thread, &result) == LOR_STATUS_OK);
    CHECK(thread.impl == NULL);
    CHECK(value == 42);
    CHECK(result == 7);

    CounterState state = {.mutex = LOR_MUTEX_INIT, .value = 0};
    CHECK(lor_mutex_init(&state.mutex) == LOR_STATUS_OK);
    LorThread workers[4] = {LOR_THREAD_INIT, LOR_THREAD_INIT, LOR_THREAD_INIT,
                            LOR_THREAD_INIT};
    for (size_t i = 0; i < 4; ++i)
        CHECK(lor_thread_start(&workers[i], increment_thread, &state) ==
              LOR_STATUS_OK);
    for (size_t i = 0; i < 4; ++i) {
        CHECK(lor_thread_join(&workers[i], &result) == LOR_STATUS_OK);
        CHECK(result == 0);
    }
    CHECK(state.value == 20000);
    lor_mutex_deinit(&state.mutex);
    return 0;
}

typedef struct CondState {
    LorMutex mutex;
    LorCond condition;
    int ready;
} CondState;

static int signal_thread(void *user) {
    CondState *state = (CondState *)user;
    if (lor_mutex_lock(&state->mutex) != LOR_STATUS_OK) return 1;
    state->ready = 1;
    if (lor_cond_signal(&state->condition) != LOR_STATUS_OK) return 1;
    return lor_mutex_unlock(&state->mutex) == LOR_STATUS_OK ? 0 : 1;
}

static int test_condition_and_time(void) {
    LorDeadline before = lor_time_now_ms();
    lor_sleep_ms(5);
    CHECK(lor_time_now_ms() >= before);
    CHECK(lor_deadline_after_ms(0) <= lor_time_now_ms());

    CondState state = {
        .mutex = LOR_MUTEX_INIT, .condition = LOR_COND_INIT, .ready = 0};
    CHECK(lor_mutex_init(&state.mutex) == LOR_STATUS_OK);
    CHECK(lor_cond_init(&state.condition) == LOR_STATUS_OK);

    LorThread thread = LOR_THREAD_INIT;
    CHECK(lor_thread_start(&thread, signal_thread, &state) == LOR_STATUS_OK);
    CHECK(lor_mutex_lock(&state.mutex) == LOR_STATUS_OK);
    LorDeadline deadline = lor_deadline_after_ms(1000);
    while (!state.ready)
        CHECK(lor_cond_wait_until(&state.condition, &state.mutex, deadline) ==
              LOR_STATUS_OK);
    CHECK(lor_mutex_unlock(&state.mutex) == LOR_STATUS_OK);
    CHECK(lor_thread_join(&thread, NULL) == LOR_STATUS_OK);

    CHECK(lor_mutex_lock(&state.mutex) == LOR_STATUS_OK);
    CHECK(lor_cond_wait_until(&state.condition, &state.mutex,
                              lor_deadline_after_ms(10)) == LOR_STATUS_TIMED_OUT);
    CHECK(lor_mutex_unlock(&state.mutex) == LOR_STATUS_OK);

    lor_cond_deinit(&state.condition);
    lor_mutex_deinit(&state.mutex);
    return 0;
}

static int test_buffered_channel(void) {
    LorChannel channel = LOR_CHANNEL_INIT;
    CHECK(lor_channel_init(&channel, int, 2) == LOR_STATUS_OK);
    CHECK(lor_channel_element_size(&channel) == sizeof(int));
    CHECK(lor_channel_capacity(&channel) == 2);
    CHECK(!lor_channel_is_closed(&channel));

    int one = 1;
    int two = 2;
    int three = 3;
    CHECK(lor_channel_send(&channel, &one) == LOR_STATUS_OK);
    CHECK(lor_channel_send(&channel, &two) == LOR_STATUS_OK);
    CHECK(lor_channel_send_until(&channel, &three, lor_deadline_after_ms(10)) ==
          LOR_STATUS_TIMED_OUT);

    int value = 0;
    CHECK(lor_channel_receive(&channel, &value) == LOR_STATUS_OK);
    CHECK(value == 1);
    CHECK(lor_channel_send(&channel, &three) == LOR_STATUS_OK);
    CHECK(lor_channel_close(&channel) == LOR_STATUS_OK);
    CHECK(lor_channel_close(&channel) == LOR_STATUS_OK);
    CHECK(lor_channel_is_closed(&channel));
    CHECK(lor_channel_send(&channel, &one) == LOR_STATUS_CLOSED);

    CHECK(lor_channel_receive(&channel, &value) == LOR_STATUS_OK);
    CHECK(value == 2);
    CHECK(lor_channel_receive(&channel, &value) == LOR_STATUS_OK);
    CHECK(value == 3);
    CHECK(lor_channel_receive(&channel, &value) == LOR_STATUS_CLOSED);
    CHECK(lor_channel_send_raw(&channel, &one, sizeof(short)) ==
          LOR_STATUS_INVALID_ARGUMENT);

    lor_channel_deinit(&channel);
    CHECK(channel.impl == NULL);
    return 0;
}

typedef struct ReceiveState {
    LorChannel *channel;
    int value;
    LorStatus status;
} ReceiveState;

typedef ReceiveState SendState;

static int receive_thread(void *user) {
    ReceiveState *state = (ReceiveState *)user;
    state->status = lor_channel_receive(state->channel, &state->value);
    return 0;
}

static int send_thread(void *user) {
    SendState *state = (SendState *)user;
    state->status = lor_channel_send(state->channel, &state->value);
    return 0;
}

static int test_rendezvous_and_close(void) {
    LorChannel channel = LOR_CHANNEL_INIT;
    CHECK(lor_channel_init(&channel, int, 0) == LOR_STATUS_OK);

    ReceiveState state = {
        .channel = &channel, .value = 0, .status = LOR_STATUS_SYSTEM_ERROR};
    LorThread thread = LOR_THREAD_INIT;
    CHECK(lor_thread_start(&thread, receive_thread, &state) == LOR_STATUS_OK);
    int value = 99;
    CHECK(lor_channel_send_until(&channel, &value, lor_deadline_after_ms(1000)) ==
          LOR_STATUS_OK);
    CHECK(lor_thread_join(&thread, NULL) == LOR_STATUS_OK);
    CHECK(state.status == LOR_STATUS_OK);
    CHECK(state.value == 99);

    state.status = LOR_STATUS_SYSTEM_ERROR;
    CHECK(lor_thread_start(&thread, receive_thread, &state) == LOR_STATUS_OK);
    lor_sleep_ms(5);
    CHECK(lor_channel_close(&channel) == LOR_STATUS_OK);
    CHECK(lor_thread_join(&thread, NULL) == LOR_STATUS_OK);
    CHECK(state.status == LOR_STATUS_CLOSED);
    lor_channel_deinit(&channel);

    channel = (LorChannel)LOR_CHANNEL_INIT;
    CHECK(lor_channel_init(&channel, int, 0) == LOR_STATUS_OK);
    SendState sender = {
        .channel = &channel, .value = 7, .status = LOR_STATUS_SYSTEM_ERROR};
    CHECK(lor_thread_start(&thread, send_thread, &sender) == LOR_STATUS_OK);
    lor_sleep_ms(5);
    CHECK(lor_channel_close(&channel) == LOR_STATUS_OK);
    CHECK(lor_thread_join(&thread, NULL) == LOR_STATUS_OK);
    CHECK(sender.status == LOR_STATUS_CLOSED);
    lor_channel_deinit(&channel);
    return 0;
}

typedef struct TaskState {
    atomic_int completed;
    atomic_int observed_cancel;
} TaskState;

static void complete_task(LorTaskContext *context, void *user) {
    TaskState *state = (TaskState *)user;
    if (!lor_task_cancelled(context)) (void)atomic_fetch_add(&state->completed, 1);
}

static void cancellation_task(LorTaskContext *context, void *user) {
    TaskState *state = (TaskState *)user;
    while (!lor_task_cancelled(context)) lor_sleep_ms(1);
    atomic_store(&state->observed_cancel, 1);
}

static int test_task_groups(void) {
    TaskState state;
    atomic_init(&state.completed, 0);
    atomic_init(&state.observed_cancel, 0);

    LorTaskGroup group = LOR_TASK_GROUP_INIT;
    CHECK(lor_task_group_init(&group) == LOR_STATUS_OK);
    for (int i = 0; i < 4; ++i)
        CHECK(lor_task_group_spawn(&group, complete_task, &state) == LOR_STATUS_OK);
    CHECK(lor_task_group_wait(&group) == LOR_STATUS_OK);
    CHECK(lor_task_group_wait(&group) == LOR_STATUS_OK);
    CHECK(atomic_load(&state.completed) == 4);
    CHECK(lor_task_group_spawn(&group, complete_task, &state) == LOR_STATUS_CLOSED);
    lor_task_group_deinit(&group);

    group = (LorTaskGroup)LOR_TASK_GROUP_INIT;
    CHECK(lor_task_group_init(&group) == LOR_STATUS_OK);
    CHECK(lor_task_group_spawn(&group, cancellation_task, &state) == LOR_STATUS_OK);
    lor_task_group_cancel(&group);
    CHECK(lor_task_group_cancelled(&group));
    CHECK(lor_task_group_wait(&group) == LOR_STATUS_OK);
    CHECK(atomic_load(&state.observed_cancel) == 1);
    lor_task_group_deinit(&group);
    CHECK(group.impl == NULL);
    return 0;
}

static int test_invalid_arguments(void) {
    LorThread thread = LOR_THREAD_INIT;
    LorMutex mutex = LOR_MUTEX_INIT;
    LorCond condition = LOR_COND_INIT;
    LorTaskGroup group = LOR_TASK_GROUP_INIT;
    LorChannel channel = LOR_CHANNEL_INIT;

    CHECK(lor_thread_start(NULL, return_value_thread, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_thread_join(&thread, NULL) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_mutex_lock(&mutex) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_cond_wait(&condition, &mutex) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_task_group_wait(&group) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_channel_init_raw(&channel, 0, 1) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_channel_close(&channel) == LOR_STATUS_INVALID_ARGUMENT);
    return 0;
}

int main(void) {
    CHECK(test_threads_and_mutex() == 0);
    CHECK(test_condition_and_time() == 0);
    CHECK(test_buffered_channel() == 0);
    CHECK(test_rendezvous_and_close() == 0);
    CHECK(test_task_groups() == 0);
    CHECK(test_invalid_arguments() == 0);

    puts("test_concurrency: ok");
    return 0;
}
