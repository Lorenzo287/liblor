// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_CLI
#define LOR_LEAKCHECK
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#define CHECK(expr)                          \
    do {                                     \
        if (!(expr)) {                       \
            fprintf(stderr, "check failed"); \
            return 1;                        \
        }                                    \
    } while (0)

int main(void) {
    CHECK(leakcheck_count() == 0);
    {
        AUTO_CLI Cli cli = CLI_INIT;
        int64_t jobs = 4;
        cli_init(&cli, "tool", NULL);
        CHECK(cli_add_int(&cli, 'j', "jobs", &jobs, NULL) == STATUS_OK);
        CHECK(leakcheck_count() != 0);
    }
    CHECK(leakcheck_count() == 0);

    puts("test_single_header_cli_leakcheck: ok");
    return 0;
}
