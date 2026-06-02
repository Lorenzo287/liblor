// SPDX-License-Identifier: MIT

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define LOR_EXE ".exe"
#define LOR_TEST_RUN ".\\.build\\test_arena.exe"
static int make_dir(const char *path) {
    if (_mkdir(path) == 0 || errno == EEXIST) { return 0; }

    perror(path);
    return 1;
}
#else
#include <sys/stat.h>
#include <sys/types.h>
#define LOR_EXE ""
#define LOR_TEST_RUN ".build/test_arena"
static int make_dir(const char *path) {
    if (mkdir(path, 0777) == 0 || errno == EEXIST) { return 0; }

    perror(path);
    return 1;
}
#endif

static int run(const char *cmd) {
    int rc = 0;

    printf("%s\n", cmd);
    fflush(stdout);
    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "command failed with exit code %d\n", rc);
        return 1;
    }

    return 0;
}

static int format_cmd(char *buf, size_t buf_size, const char *fmt, const char *cc) {
    int len = snprintf(buf, buf_size, fmt, cc);

    if (len < 0 || (size_t)len >= buf_size) {
        fprintf(stderr, "build command is too long\n");
        return 1;
    }

    return 0;
}

static const char *compiler(void) {
    const char *cc = getenv("CC");
    return cc != NULL && cc[0] != '\0' ? cc : "clang";
}

static int compile_arena(const char *cc) {
    char cmd[1024];

    if (format_cmd(cmd, sizeof(cmd),
                   "%s -std=c11 -Wall -Wextra -Wpedantic -Iinclude -c src/arena.c "
                   "-o .build/arena.o",
                   cc)) {
        return 1;
    }

    return run(cmd);
}

static int build_test(const char *cc) {
    char cmd[1024];

    if (format_cmd(cmd, sizeof(cmd),
                   "%s -std=c11 -Wall -Wextra -Wpedantic -Iinclude .build/arena.o "
                   "tests/test_arena.c -o .build/test_arena" LOR_EXE,
                   cc)) {
        return 1;
    }

    return run(cmd);
}

static int run_test(void) {
    return run(LOR_TEST_RUN);
}

static int build_example(const char *cc) {
    char cmd[1024];

    if (format_cmd(cmd, sizeof(cmd),
                   "%s -std=c11 -Wall -Wextra -Wpedantic -Iinclude .build/arena.o "
                   "examples/arena_basic.c -o .build/arena_basic" LOR_EXE,
                   cc)) {
        return 1;
    }

    return run(cmd);
}

static void usage(const char *program) {
fprintf(stderr, "usage: %s [test|example|all]\n", program);
}

int main(int argc, char **argv) {
const char *target = argc > 1 ? argv[1] : "test";
const char *cc = compiler();

if (make_dir(".build")) { return 1; }

if (compile_arena(cc)) { return 1; }

if (strcmp(target, "test") == 0) { return build_test(cc) || run_test(); }

if (strcmp(target, "example") == 0) { return build_example(cc); }

if (strcmp(target, "all") == 0) {
	return build_test(cc) || run_test() || build_example(cc);
}

usage(argv[0]);
return 1;
}
