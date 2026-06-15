// SPDX-License-Identifier: MIT

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lor/trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lor_test.h"

#if defined(__GNUC__) || defined(__clang__)
#define TEST_NOINLINE __attribute__((noinline))
#else
#define TEST_NOINLINE
#endif

static volatile int test_trace_auto_sink;

static TEST_NOINLINE int auto_leaf(int value) {
    test_trace_auto_sink += value;
    return value + 1;
}

static TEST_NOINLINE int auto_parent(int value) {
    return auto_leaf(value) + auto_leaf(value + 1);
}

static char *read_trace(const char *path) {
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

int main(void) {
    const char *path = ".build/trace_auto.json";
    LorTrace trace = LOR_TRACE_INIT;
    LorTraceThread thread = LOR_TRACE_THREAD_INIT;
    LorTraceConfig config = LOR_TRACE_CONFIG_INIT;
    config.process_name = "trace auto test";

    CHECK(LOR_TRACE_AUTO_SUPPORTED);
    CHECK(lor_trace_init_file(&trace, path, config) == LOR_STATUS_OK);
    CHECK(lor_trace_thread_init(&thread, &trace, "main") == LOR_STATUS_OK);
    CHECK(lor_trace_auto_bind(&thread) == LOR_STATUS_OK);
    CHECK(lor_trace_auto_is_enabled());
    CHECK(auto_parent(3) == 9);
    lor_trace_auto_unbind();
    CHECK(!lor_trace_auto_is_enabled());

    LorTraceThreadStats stats = lor_trace_thread_stats(&thread);
    CHECK(stats.recorded_events >= 6);
    CHECK(stats.open_zones == 0);
    CHECK(lor_trace_thread_finish(&thread) == LOR_STATUS_OK);
    CHECK(lor_trace_finish(&trace) == LOR_STATUS_OK);

    char *text = read_trace(path);
    CHECK(text != NULL);
    CHECK(strstr(text, "\"cat\":\"lor.auto\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"B\"") != NULL);
    CHECK(strstr(text, "\"ph\":\"E\"") != NULL);
    free(text);

    puts("test_trace_auto: ok");
    return 0;
}
