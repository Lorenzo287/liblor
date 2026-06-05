// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_MEMORY
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                            \
    do {                                       \
        if (!(expr)) {                         \
            fprintf(stderr, "check failed\n"); \
            return 1;                          \
        }                                      \
    } while (0)

static int test_lazy_arena_leak_location(void) {
    Arena arena = ARENA_INIT;
    FILE *report = NULL;
    char text[512] = {0};
    char expected_size[64] = {0};
    size_t nread = 0;

    CHECK(arena_alloc(&arena, 32) != NULL);
    CHECK(snprintf(expected_size, sizeof(expected_size), "LEAK arena: %zu bytes",
                   ARENA_DEFAULT_BLOCK_SIZE) > 0);
    report = fopen(".build/lor_single_header_leak_report.txt", "wb+");
    CHECK(report != NULL);
    CHECK(leakcheck_report(report) >= 1);
    CHECK(fflush(report) == 0);
    CHECK(fseek(report, 0, SEEK_SET) == 0);
    nread = fread(text, 1, sizeof(text) - 1u, report);
    CHECK(ferror(report) == 0);
    text[nread] = '\0';
    CHECK(fclose(report) == 0);

    CHECK(strstr(text, "?:0") == NULL);
    CHECK(strstr(text, "LEAK arena: 0 bytes") == NULL);
    CHECK(strstr(text, expected_size) != NULL);
    CHECK(strstr(text, "test_single_header.c") != NULL);

    arena_deinit(&arena);
    CHECK(leakcheck_count() == 0);
    return 0;
}

int main(void) {
    Arena arena = ARENA_INIT;
    Arena configured = ARENA_INIT;
    ArenaMark mark = ARENA_MARK_INIT;
    int *values = (int *)arena_alloc_array_zero(&arena, 4, sizeof(*values));
    int *configured_value = NULL;
    char *scratch = NULL;
    char *heap = NULL;

    CHECK(values != NULL);
    CHECK(arena_used(&arena) >= 4 * sizeof(*values));
    mark = arena_mark(&arena);
    scratch = (char *)arena_alloc(&arena, 32);
    CHECK(scratch != NULL);

    arena_rewind(&arena, mark);
    arena_deinit(&arena);

    CHECK(arena_init_config(&configured, (ArenaConfig){.block_size = 128}));
    configured_value =
        (int *)arena_alloc_zero(&configured, sizeof(*configured_value));
    CHECK(configured_value != NULL);
    CHECK(*configured_value == 0);
    arena_deinit(&configured);

    heap = (char *)malloc(16);
    CHECK(heap != NULL);
    CHECK(leakcheck_count() == 1);
    free(heap);
    CHECK(leakcheck_count() == 0);

    CHECK(test_lazy_arena_leak_location() == 0);

    puts("test_single_header: ok");
    return 0;
}
