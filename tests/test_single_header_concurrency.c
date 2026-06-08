// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_CONCURRENCY
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

static int worker(void *user) {
    Channel *channel = (Channel *)user;
    int value = 12;
    return channel_send(channel, &value) == STATUS_OK ? 0 : 1;
}

int main(void) {
    Channel channel = CHANNEL_INIT;
    CHECK(channel_init(&channel, int, 1) == STATUS_OK);

    Thread thread = THREAD_INIT;
    CHECK(thread_start(&thread, worker, &channel) == STATUS_OK);

    int value = 0;
    CHECK(channel_receive(&channel, &value) == STATUS_OK);
    CHECK(value == 12);
    CHECK(thread_join(&thread, NULL) == STATUS_OK);

    channel_deinit(&channel);
    puts("test_single_header_concurrency: ok");
    return 0;
}
