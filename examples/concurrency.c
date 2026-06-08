// SPDX-License-Identifier: MIT

#include "lor/concurrency.h"

#include <stdio.h>

static int produce(void *user) {
    LorChannel *channel = (LorChannel *)user;
    for (int value = 1; value <= 5; ++value) {
        if (lor_channel_send(channel, &value) != LOR_STATUS_OK) return 1;
    }
    return lor_channel_close(channel) == LOR_STATUS_OK ? 0 : 1;
}

int main(void) {
    LorChannel values = LOR_CHANNEL_INIT;
    if (lor_channel_init(&values, int, 2) != LOR_STATUS_OK) return 1;

    LorThread producer = LOR_THREAD_INIT;
    if (lor_thread_start(&producer, produce, &values) != LOR_STATUS_OK) {
        lor_channel_deinit(&values);
        return 1;
    }

    int value;
    while (lor_channel_receive(&values, &value) == LOR_STATUS_OK)
        printf("%d\n", value * value);

    int result = 1;
    if (lor_thread_join(&producer, &result) != LOR_STATUS_OK) result = 1;
    lor_channel_deinit(&values);
    return result;
}
