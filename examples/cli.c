// SPDX-License-Identifier: MIT

#include "lor/cli.h"

#include <inttypes.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    LOR_AUTO_CLI LorCli cli = LOR_CLI_INIT;
    LOR_AUTO_ARRAY LorStringView *defines = LOR_ARRAY_INIT;

    size_t verbosity = 0;
    int64_t jobs = 1;
    LorStringView output = LOR_SV_LITERAL("stdout");
    LorStringView input = LOR_STRING_VIEW_INIT;

    lor_cli_init(&cli, NULL, "Compile one input file.");
    if (lor_cli_add_count(&cli, 'v', "verbose", &verbosity,
                          "Increase diagnostic output") != LOR_STATUS_OK ||
        lor_cli_add_int(&cli, 'j', "jobs", &jobs, "Number of parallel jobs") !=
            LOR_STATUS_OK ||
        lor_cli_add_string(&cli, 'o', "output", &output, "Output file") !=
            LOR_STATUS_OK)
        return 1;

    LorCliOption define_option = LOR_CLI_OPTION_INIT;
    define_option.short_name = 'D';
    define_option.long_name = "define";
    define_option.value_name = "NAME=VALUE";
    define_option.help = "Add a preprocessor definition";
    if (lor_cli_add_strings_option(&cli, define_option, &defines) != LOR_STATUS_OK)
        return 1;

    LorCliPositional input_positional = LOR_CLI_POSITIONAL_INIT;
    input_positional.name = "input";
    input_positional.help = "Source file to compile";
    if (lor_cli_add_positional_string(&cli, input_positional, &input) !=
        LOR_STATUS_OK)
        return 1;

    LorCliResult result = lor_cli_parse(&cli, argc, argv);
    if (result.code == LOR_CLI_HELP_REQUESTED) {
        lor_cli_print_help(&cli);
        return 0;
    }
    if (result.code != LOR_CLI_OK) {
        lor_cli_fprint_error(stderr, result);
        fprintf(stderr, "Try '%s --help' for usage.\n",
                cli.program_name != NULL ? cli.program_name : "program");
        return 2;
    }

    printf("verbosity: %zu\njobs: %" PRId64 "\noutput: ", verbosity, jobs);
    lor_sv_fprint(stdout, output);
    fputs("\ninput: ", stdout);
    lor_sv_fprint(stdout, input);
    fputc('\n', stdout);

    for (size_t i = 0; i < lor_array_size(defines); ++i) {
        fputs("define: ", stdout);
        lor_sv_fprint(stdout, defines[i]);
        fputc('\n', stdout);
    }
    return 0;
}
