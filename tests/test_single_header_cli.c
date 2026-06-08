// SPDX-License-Identifier: MIT

#define LOR_IMPLEMENTATION
#define LOR_ENABLE_CLI
#define LOR_STRIP_PREFIX
#include "../lor.h"

#include <stdio.h>

#include "lor_test.h"

int main(void) {
    Cli cli = CLI_INIT;
    bool verbose = false;
    int64_t jobs = 1;
    StringView input = STRING_VIEW_INIT;

    cli_init(&cli, "tool", NULL);
    CHECK(cli_add_flag(&cli, 'v', "verbose", &verbose, NULL) == STATUS_OK);
    CHECK(cli_add_int(&cli, 'j', "jobs", &jobs, NULL) == STATUS_OK);

    CliPositional positional = CLI_POSITIONAL_INIT;
    positional.name = "input";
    CHECK(cli_add_positional_string(&cli, positional, &input) == STATUS_OK);

    char *argv[] = {"tool", "-v", "--jobs=4", "input.txt"};
    CliResult result = cli_parse(&cli, 4, argv);
    CHECK(result.code == CLI_OK);
    CHECK(verbose);
    CHECK(jobs == 4);
    CHECK(sv_equal(input, (StringView)SV_LITERAL("input.txt")));

    cli_deinit(&cli);
    puts("test_single_header_cli: ok");
    return 0;
}
