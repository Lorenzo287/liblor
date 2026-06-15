// SPDX-License-Identifier: MIT

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lor/trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lor_test.h"

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *text = (char *)malloc((size_t)length + 1u);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }
    size_t count = fread(text, 1, (size_t)length, file);
    fclose(file);
    if (count != (size_t)length) {
        free(text);
        return NULL;
    }
    text[count] = '\0';
    return text;
}

static void traced_scope(LorTraceThread *thread) {
#if LOR_TRACE_SCOPE_SUPPORTED
    LOR_TRACE_SCOPE(thread, "cleanup scope");
    LOR_TRACE_INSTANT(thread, "inside");
#else
    LOR_TRACE_BEGIN(thread, "cleanup scope");
    LOR_TRACE_INSTANT(thread, "inside");
    LOR_TRACE_END(thread);
#endif
}

static int test_manual_trace(void) {
    const char *path = ".build/trace_manual.json";
    LorTrace trace = LOR_TRACE_INIT;
    LorTraceConfig config = LOR_TRACE_CONFIG_INIT;
    config.buffer_size = LOR_TRACE_MIN_BUFFER_SIZE;
    config.process_name = "trace test";
    CHECK(lor_trace_init_file(&trace, path, config) == LOR_STATUS_OK);

    LorTraceThread thread = LOR_TRACE_THREAD_INIT;
    CHECK(lor_trace_thread_init(&thread, &trace, "main \"thread\"") ==
          LOR_STATUS_OK);

    LOR_TRACE_BEGIN(&thread, "outer");
    LOR_TRACE_INSTANT(&thread, "tick\n");
    LOR_TRACE_COUNTER(&thread, "items", -7);
    traced_scope(&thread);
    LOR_TRACE_END(&thread);

    LorTraceThreadStats stats = lor_trace_thread_stats(&thread);
    CHECK(stats.recorded_events == 7);
    CHECK(stats.dropped_events == 0);
    CHECK(stats.open_zones == 0);
    CHECK(stats.buffer_capacity == LOR_TRACE_MIN_BUFFER_SIZE);
    CHECK(lor_trace_thread_flush(&thread) == LOR_STATUS_OK);
    CHECK(lor_trace_thread_finish(&thread) == LOR_STATUS_OK);
    CHECK(lor_trace_finish(&trace) == LOR_STATUS_OK);

    char *text = read_file(path);
    CHECK(text != NULL);
    CHECK(text[0] == '[');
    CHECK(strstr(text, "\"name\":\"process_name\"") != NULL);
    CHECK(strstr(text, "main \\\"thread\\\"") != NULL);
    CHECK(strstr(text, "\"name\":\"outer\"") != NULL);
    CHECK(strstr(text, "\"name\":\"tick\\n\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"B\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"E\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"i\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"C\"") != NULL);
    CHECK(strstr(text, "\"value\":-7") != NULL);
    CHECK(text[strlen(text) - 2u] == ']');
    free(text);
    return 0;
}

static int test_drop_mode_and_unbalanced_finish(void) {
    const char *path = ".build/trace_drop.json";
    LorTrace trace = LOR_TRACE_INIT;
    LorTraceConfig config = LOR_TRACE_CONFIG_INIT;
    config.buffer_size = LOR_TRACE_MIN_BUFFER_SIZE;
    config.buffer_mode = LOR_TRACE_BUFFER_DROP;
    CHECK(lor_trace_init_file(&trace, path, config) == LOR_STATUS_OK);

    LorTraceThread thread = LOR_TRACE_THREAD_INIT;
    CHECK(lor_trace_thread_init(&thread, &trace, "drop") == LOR_STATUS_OK);
    LOR_TRACE_BEGIN(&thread, "unclosed");
    for (size_t i = 0; i < 100; ++i)
        LOR_TRACE_INSTANT(&thread, "event that eventually fills the buffer");

    LorTraceThreadStats stats = lor_trace_thread_stats(&thread);
    CHECK(stats.open_zones == 1);
    CHECK(stats.dropped_events != 0);
    CHECK(lor_trace_thread_finish(&thread) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_finish(&trace) == LOR_STATUS_OK);

    char *text = read_file(path);
    CHECK(text != NULL);
    CHECK(strstr(text, "\"name\":\"unclosed\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"E\"") != NULL);
    CHECK(text[strlen(text) - 2u] == ']');
    free(text);
    return 0;
}

static int test_invalid_state(void) {
    LorTrace trace = LOR_TRACE_INIT;
    LorTraceThread thread = LOR_TRACE_THREAD_INIT;
    LorTraceConfig config = LOR_TRACE_CONFIG_INIT;
    config.buffer_size = 1;

    CHECK(lor_trace_init_file(NULL, "unused", config) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_init_file(&trace, "unused", config) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_thread_init(&thread, &trace, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_thread_flush(&thread) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_thread_finish(&thread) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_finish(&trace) == LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_trace_auto_bind(&thread) == LOR_STATUS_SYSTEM_ERROR);
    return 0;
}

int main(void) {
    CHECK(test_manual_trace() == 0);
    CHECK(test_drop_mode_and_unbalanced_finish() == 0);
    CHECK(test_invalid_state() == 0);

    puts("test_trace: ok");
    return 0;
}
