// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_TRACE
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    Trace trace = TRACE_INIT;
    TraceConfig config = TRACE_CONFIG_INIT;
    config.process_name = "single header";
    CHECK(trace_init_file(&trace, ".build/trace_single_header.json", config) ==
          STATUS_OK);

    TraceThread thread = TRACE_THREAD_INIT;
    CHECK(trace_thread_init(&thread, &trace, "main") == STATUS_OK);
    TRACE_BEGIN(&thread, "work");
    TRACE_COUNTER(&thread, "count", 3);
    TRACE_END(&thread);
    CHECK(trace_thread_finish(&thread) == STATUS_OK);
    CHECK(trace_finish(&trace) == STATUS_OK);

    puts("test_single_header_trace: ok");
    return 0;
}
