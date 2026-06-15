// SPDX-License-Identifier: MIT

#include "lor/trace.h"

#include <stdio.h>

static unsigned long fibonacci(LorTraceThread *trace, unsigned int n) {
    LOR_TRACE_BEGIN(trace, __func__);
    unsigned long result =
        n < 2 ? n : fibonacci(trace, n - 1) + fibonacci(trace, n - 2);
    LOR_TRACE_END(trace);
    return result;
}

int main(void) {
    LorTrace trace = LOR_TRACE_INIT;
    LorTraceConfig config = LOR_TRACE_CONFIG_INIT;
    config.process_name = "liblor trace example";
    if (lor_trace_init_file(&trace, "trace.json", config) != LOR_STATUS_OK) {
        fputs("could not create trace.json\n", stderr);
        return 1;
    }

    LorTraceThread thread = LOR_TRACE_THREAD_INIT;
    if (lor_trace_thread_init(&thread, &trace, "main") != LOR_STATUS_OK) {
        lor_trace_deinit(&trace);
        return 1;
    }

    LOR_TRACE_BEGIN(&thread, "example");
    unsigned long result = fibonacci(&thread, 10);
    LOR_TRACE_COUNTER(&thread, "result", (int64_t)result);
    LOR_TRACE_END(&thread);

    LorStatus status = lor_trace_thread_finish(&thread);
    if (status == LOR_STATUS_OK) status = lor_trace_finish(&trace);
    if (status != LOR_STATUS_OK) {
        fputs("could not finish trace\n", stderr);
        return 1;
    }

    printf("fibonacci(10) = %lu; wrote trace.json\n", result);
    return 0;
}
