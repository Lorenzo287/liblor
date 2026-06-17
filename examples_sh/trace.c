#include <stdio.h>

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_TRACE
#define LOR_TRACE_AUTO
#include "../lor.h"

// clang -g -ldbghelp -finstrument-functions trace.c -o trace.exe

static unsigned long fibonacci(unsigned int n) {
    unsigned long result =
        n < 2 ? n : fibonacci(n - 1) + fibonacci(n - 2);
    return result;
}

int main(void) {
    LorTrace trace = LOR_TRACE_INIT;
    LorTraceThread thread = LOR_TRACE_THREAD_INIT;
    LorTraceConfig config = LOR_TRACE_CONFIG_INIT;

    lor_trace_init_file(&trace, "trace_auto.json", config);
    lor_trace_thread_init(&thread, &trace, "main");

    lor_trace_auto_bind(&thread);
    unsigned long result = fibonacci(10);
    lor_trace_auto_unbind();

    lor_trace_thread_finish(&thread);
    lor_trace_finish(&trace);

    printf("fibonacci(10) = %lu; wrote trace_auto.json\n", result);
    return 0;
}
