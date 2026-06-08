// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "lor/cli.h"

#include "lor/memory.h"

#include <stdio.h>
#include <string.h>

#include "lor_test.h"

static int stream_text(FILE *stream, char *buffer, size_t capacity) {
    CHECK(stream != NULL);
    CHECK(capacity != 0);
    CHECK(fflush(stream) == 0);
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    size_t size = fread(buffer, 1, capacity - 1u, stream);
    CHECK(ferror(stream) == 0);
    buffer[size] = '\0';
    return 0;
}

static int test_parse_options_and_positionals(void) {
    LorCli cli = LOR_CLI_INIT;
    lor_cli_init(&cli, "convert", "Convert an input file.");

    bool quiet = false;
    size_t verbosity = 0;
    LorStringView output = LOR_SV_LITERAL("stdout");
    int64_t jobs = 1;
    double scale = 1.0;
    LorStringView *tags = LOR_ARRAY_INIT;
    LorStringView input = LOR_STRING_VIEW_INIT;
    LorStringView *extra = LOR_ARRAY_INIT;

    CHECK(lor_cli_add_flag(&cli, 'q', "quiet", &quiet, "Suppress normal output") ==
          LOR_STATUS_OK);
    CHECK(lor_cli_add_count(&cli, 'v', "verbose", &verbosity,
                            "Increase verbosity") == LOR_STATUS_OK);

    LorCliOption output_option = LOR_CLI_OPTION_INIT;
    output_option.short_name = 'o';
    output_option.long_name = "output";
    output_option.value_name = "FILE";
    output_option.help = "Write output to FILE";
    output_option.required = 1;
    CHECK(lor_cli_add_string_option(&cli, output_option, &output) == LOR_STATUS_OK);
    CHECK(lor_cli_add_int(&cli, 'j', "jobs", &jobs, "Number of worker jobs") ==
          LOR_STATUS_OK);
    CHECK(lor_cli_add_double(&cli, 's', "scale", &scale, "Output scale") ==
          LOR_STATUS_OK);

    LorCliOption tag_option = LOR_CLI_OPTION_INIT;
    tag_option.long_name = "tag";
    tag_option.value_name = "NAME";
    tag_option.help = "Attach a tag";
    CHECK(lor_cli_add_strings_option(&cli, tag_option, &tags) == LOR_STATUS_OK);

    LorCliPositional input_positional = LOR_CLI_POSITIONAL_INIT;
    input_positional.name = "input";
    input_positional.help = "Input file";
    CHECK(lor_cli_add_positional_string(&cli, input_positional, &input) ==
          LOR_STATUS_OK);
    CHECK(lor_cli_add_positionals(&cli, "extra", &extra, "Additional input files") ==
          LOR_STATUS_OK);

    char *argv[] = {
        "convert", "-q",       "-vv",        "--output=result.txt",
        "-j",      "4",        "--scale",    "2.5",
        "--tag",   "primary",  "--tag=fast", "input.txt",
        "--",      "-literal", "tail",
    };
    LorCliResult result =
        lor_cli_parse(&cli, (int)(sizeof(argv) / sizeof(argv[0])), argv);
    CHECK(result.code == LOR_CLI_OK);
    CHECK(quiet);
    CHECK(verbosity == 2);
    CHECK(lor_sv_equal(output, (LorStringView)LOR_SV_LITERAL("result.txt")));
    CHECK(jobs == 4);
    CHECK(scale == 2.5);
    CHECK(lor_array_size(tags) == 2);
    CHECK(lor_sv_equal(tags[0], (LorStringView)LOR_SV_LITERAL("primary")));
    CHECK(lor_sv_equal(tags[1], (LorStringView)LOR_SV_LITERAL("fast")));
    CHECK(lor_sv_equal(input, (LorStringView)LOR_SV_LITERAL("input.txt")));
    CHECK(lor_array_size(extra) == 2);
    CHECK(lor_sv_equal(extra[0], (LorStringView)LOR_SV_LITERAL("-literal")));
    CHECK(lor_sv_equal(extra[1], (LorStringView)LOR_SV_LITERAL("tail")));

    CHECK(lor_cli_parse(&cli, 0, NULL).code == LOR_CLI_INVALID_STATE);

    lor_array_deinit(&extra);
    lor_array_deinit(&tags);
    lor_cli_deinit(&cli);
    return 0;
}

static int test_attached_values_and_negative_positionals(void) {
    LorCli cli = LOR_CLI_INIT;
    lor_cli_init(&cli, "calculate", NULL);

    LorStringView output = LOR_STRING_VIEW_INIT;
    int64_t jobs = 0;
    int64_t value = 0;
    CHECK(lor_cli_add_string(&cli, 'o', "output", &output, NULL) == LOR_STATUS_OK);
    CHECK(lor_cli_add_int(&cli, 'j', "jobs", &jobs, NULL) == LOR_STATUS_OK);

    LorCliPositional positional = LOR_CLI_POSITIONAL_INIT;
    positional.name = "value";
    CHECK(lor_cli_add_positional_int(&cli, positional, &value) == LOR_STATUS_OK);

    char *argv[] = {"calculate", "-ofile.txt", "-j=3", "-12"};
    LorCliResult result =
        lor_cli_parse(&cli, (int)(sizeof(argv) / sizeof(argv[0])), argv);
    CHECK(result.code == LOR_CLI_OK);
    CHECK(lor_sv_equal(output, (LorStringView)LOR_SV_LITERAL("file.txt")));
    CHECK(jobs == 3);
    CHECK(value == -12);

    lor_cli_deinit(&cli);
    return 0;
}

static int test_repeated_numeric_options(void) {
    LorCli cli = LOR_CLI_INIT;
    lor_cli_init(&cli, "numbers", NULL);

    int64_t *ints = LOR_ARRAY_INIT;
    double *doubles = LOR_ARRAY_INIT;
    size_t count = 0;

    LorCliOption int_option = LOR_CLI_OPTION_INIT;
    int_option.short_name = 'n';
    int_option.long_name = "number";
    CHECK(lor_cli_add_ints_option(&cli, int_option, &ints) == LOR_STATUS_OK);

    LorCliOption double_option = LOR_CLI_OPTION_INIT;
    double_option.long_name = "ratio";
    CHECK(lor_cli_add_doubles_option(&cli, double_option, &doubles) ==
          LOR_STATUS_OK);

    LorCliOption count_option = LOR_CLI_OPTION_INIT;
    count_option.short_name = 'v';
    CHECK(lor_cli_add_count_option(&cli, count_option, &count) == LOR_STATUS_OK);

    char *argv[] = {
        "numbers", "-n", "10", "--number=20", "--ratio", "0.5", "--ratio=2", "-vv",
    };
    LorCliResult result =
        lor_cli_parse(&cli, (int)(sizeof(argv) / sizeof(argv[0])), argv);
    CHECK(result.code == LOR_CLI_OK);
    CHECK(lor_array_size(ints) == 2);
    CHECK(ints[0] == 10 && ints[1] == 20);
    CHECK(lor_array_size(doubles) == 2);
    CHECK(doubles[0] == 0.5 && doubles[1] == 2.0);
    CHECK(count == 2);

    lor_array_deinit(&doubles);
    lor_array_deinit(&ints);
    lor_cli_deinit(&cli);
    return 0;
}

static int test_help_and_error_output(void) {
    LorCli cli = LOR_CLI_INIT;
    lor_cli_init(&cli, "archive", "Create an archive.");

    bool force = false;
    int64_t level = 6;
    LorStringView input = LOR_STRING_VIEW_INIT;
    CHECK(lor_cli_add_flag(&cli, 'f', "force", &force, "Overwrite output") ==
          LOR_STATUS_OK);
    CHECK(lor_cli_add_int(&cli, 'l', "level", &level, "Compression level") ==
          LOR_STATUS_OK);
    LorCliPositional positional = LOR_CLI_POSITIONAL_INIT;
    positional.name = "input";
    positional.help = "Input directory";
    CHECK(lor_cli_add_positional_string(&cli, positional, &input) == LOR_STATUS_OK);

    FILE *help = tmpfile();
    CHECK(help != NULL);
    CHECK(lor_cli_fprint_help(&cli, help));
    char text[2048];
    CHECK(stream_text(help, text, sizeof(text)) == 0);
    CHECK(strstr(text, "Usage: archive [options] <input>") != NULL);
    CHECK(strstr(text, "Create an archive.") != NULL);
    CHECK(strstr(text, "-f, --force") != NULL);
    CHECK(strstr(text, "-l, --level INT") != NULL);
    CHECK(strstr(text, "(default: 6)") != NULL);
    CHECK(strstr(text, "-h, --help") != NULL);
    CHECK(strstr(text, "Arguments:") != NULL);
    CHECK(fclose(help) == 0);

    char *argv[] = {"archive", "--help"};
    LorCliResult result = lor_cli_parse(&cli, 2, argv);
    CHECK(result.code == LOR_CLI_HELP_REQUESTED);
    CHECK(result.argv_index == 1);

    LorCliResult error = {
        .code = LOR_CLI_UNKNOWN_OPTION,
        .argv_index = 1,
        .token = LOR_SV_LITERAL("--unknown"),
    };
    FILE *errors = tmpfile();
    CHECK(errors != NULL);
    CHECK(lor_cli_fprint_error(errors, error));
    CHECK(stream_text(errors, text, sizeof(text)) == 0);
    CHECK(strcmp(text, "error: unknown option '--unknown'\n") == 0);
    CHECK(fclose(errors) == 0);

    lor_cli_deinit(&cli);
    return 0;
}

static int test_parse_errors(void) {
    {
        LorCli cli = LOR_CLI_INIT;
        bool flag = false;
        lor_cli_init(&cli, "tool", NULL);
        CHECK(lor_cli_add_flag(&cli, 'f', "flag", &flag, NULL) == LOR_STATUS_OK);
        char *argv[] = {"tool", "--flag=yes"};
        LorCliResult result = lor_cli_parse(&cli, 2, argv);
        CHECK(result.code == LOR_CLI_UNEXPECTED_VALUE);
        CHECK(result.argv_index == 1);
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        int64_t number = 0;
        lor_cli_init(&cli, "tool", NULL);
        CHECK(lor_cli_add_int(&cli, 'n', "number", &number, NULL) == LOR_STATUS_OK);
        char *argv[] = {"tool", "--number", "nope"};
        LorCliResult result = lor_cli_parse(&cli, 3, argv);
        CHECK(result.code == LOR_CLI_INVALID_VALUE);
        CHECK(result.argv_index == 1);
        CHECK(lor_sv_equal(result.token, (LorStringView)LOR_SV_LITERAL("nope")));
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        LorStringView value = LOR_STRING_VIEW_INIT;
        lor_cli_init(&cli, "tool", NULL);
        CHECK(lor_cli_add_string(&cli, 'o', "output", &value, NULL) ==
              LOR_STATUS_OK);
        char *argv[] = {"tool", "-o"};
        CHECK(lor_cli_parse(&cli, 2, argv).code == LOR_CLI_MISSING_VALUE);
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        bool flag = false;
        lor_cli_init(&cli, "tool", NULL);
        CHECK(lor_cli_add_flag(&cli, 'f', NULL, &flag, NULL) == LOR_STATUS_OK);
        char *argv[] = {"tool", "-f", "-f"};
        LorCliResult result = lor_cli_parse(&cli, 3, argv);
        CHECK(result.code == LOR_CLI_DUPLICATE_OPTION);
        CHECK(result.short_name == 'f');
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        LorStringView output = LOR_STRING_VIEW_INIT;
        lor_cli_init(&cli, "tool", NULL);
        LorCliOption option = LOR_CLI_OPTION_INIT;
        option.short_name = 'o';
        option.required = 1;
        CHECK(lor_cli_add_string_option(&cli, option, &output) == LOR_STATUS_OK);
        char *argv[] = {"tool"};
        LorCliResult result = lor_cli_parse(&cli, 1, argv);
        CHECK(result.code == LOR_CLI_MISSING_REQUIRED_OPTION);
        CHECK(result.short_name == 'o');
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        LorStringView input = LOR_STRING_VIEW_INIT;
        lor_cli_init(&cli, "tool", NULL);
        LorCliPositional positional = LOR_CLI_POSITIONAL_INIT;
        positional.name = "input";
        CHECK(lor_cli_add_positional_string(&cli, positional, &input) ==
              LOR_STATUS_OK);
        char *argv[] = {"tool"};
        CHECK(lor_cli_parse(&cli, 1, argv).code == LOR_CLI_MISSING_POSITIONAL);
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        lor_cli_init(&cli, "tool", NULL);
        char *argv[] = {"tool", "extra"};
        CHECK(lor_cli_parse(&cli, 2, argv).code == LOR_CLI_UNEXPECTED_POSITIONAL);
        lor_cli_deinit(&cli);
    }
    {
        LorCli cli = LOR_CLI_INIT;
        lor_cli_init(&cli, "tool", NULL);
        char *argv[] = {"tool", "--unknown"};
        CHECK(lor_cli_parse(&cli, 2, argv).code == LOR_CLI_UNKNOWN_OPTION);
        lor_cli_deinit(&cli);
    }
    return 0;
}

static int test_registration_validation(void) {
    LorCli cli = LOR_CLI_INIT;
    lor_cli_init(&cli, "tool", NULL);
    bool flag = false;
    int64_t number = 0;
    CHECK(lor_cli_add_flag(&cli, 'h', "other", &flag, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_cli_add_flag(&cli, 'x', "help", &flag, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_cli_add_flag(&cli, 'x', "execute", &flag, NULL) == LOR_STATUS_OK);
    CHECK(lor_cli_add_int(&cli, 'x', "number", &number, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);
    CHECK(lor_cli_add_int(&cli, 'n', "execute", &number, NULL) ==
          LOR_STATUS_INVALID_ARGUMENT);

    LorStringView optional_value = LOR_STRING_VIEW_INIT;
    LorStringView required_value = LOR_STRING_VIEW_INIT;
    LorCliPositional optional = LOR_CLI_POSITIONAL_INIT;
    optional.name = "optional";
    optional.required = 0;
    CHECK(lor_cli_add_positional_string(&cli, optional, &optional_value) ==
          LOR_STATUS_OK);
    LorCliPositional required = LOR_CLI_POSITIONAL_INIT;
    required.name = "required";
    CHECK(lor_cli_add_positional_string(&cli, required, &required_value) ==
          LOR_STATUS_INVALID_ARGUMENT);

    LorStringView *values = LOR_ARRAY_INIT;
    CHECK(lor_array_push_as(values, LorStringView, .data = "existing", .size = 8) ==
          LOR_STATUS_OK);
    LorCliOption option = LOR_CLI_OPTION_INIT;
    option.long_name = "value";
    CHECK(lor_cli_add_strings_option(&cli, option, &values) ==
          LOR_STATUS_INVALID_ARGUMENT);

    lor_array_deinit(&values);
    lor_cli_deinit(&cli);
    return 0;
}

static int test_code_names_and_cleanup(void) {
    CHECK(strcmp(lor_cli_code_name(LOR_CLI_OK), "ok") == 0);
    CHECK(strcmp(lor_cli_code_name(LOR_CLI_HELP_REQUESTED), "help requested") == 0);
    CHECK(strcmp(lor_cli_code_name((LorCliCode)99), "unknown") == 0);

#if defined(LOR_LEAKCHECK)
    size_t before = lor_leakcheck_count();
    {
        LOR_AUTO_CLI LorCli cli = LOR_CLI_INIT;
        int64_t jobs = 4;
        lor_cli_init(&cli, "tool", NULL);
        CHECK(lor_cli_add_int(&cli, 'j', "jobs", &jobs, NULL) == LOR_STATUS_OK);
        CHECK(lor_leakcheck_count() > before);
    }
    CHECK(lor_leakcheck_count() == before);
#endif

    lor_cli_deinit(NULL);
    return 0;
}

int main(void) {
    CHECK(test_parse_options_and_positionals() == 0);
    CHECK(test_attached_values_and_negative_positionals() == 0);
    CHECK(test_repeated_numeric_options() == 0);
    CHECK(test_help_and_error_output() == 0);
    CHECK(test_parse_errors() == 0);
    CHECK(test_registration_validation() == 0);
    CHECK(test_code_names_and_cleanup() == 0);

    puts("test_cli: ok");
    return 0;
}
