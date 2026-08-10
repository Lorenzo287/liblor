// SPDX-License-Identifier: MIT

#ifndef LOR_CLI_H
#define LOR_CLI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lor/array.h"
#include "lor/status.h"
#include "lor/string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LorCli {
    void *impl_options;
    void *impl_positionals;
    const char *program_name;
    const char *description;
    int parsed;
} LorCli;

#define LOR_CLI_INIT {NULL, NULL, NULL, NULL, 0}

typedef struct LorCliOption {
    char short_name;
    const char *long_name;
    const char *value_name;
    const char *help;
    int required;
    int repeatable;
    int show_default;
} LorCliOption;

#define LOR_CLI_OPTION_INIT {0, NULL, NULL, NULL, 0, 0, 1}

typedef struct LorCliPositional {
    const char *name;
    const char *help;
    int required;
} LorCliPositional;

#define LOR_CLI_POSITIONAL_INIT {NULL, NULL, 1}

typedef enum LorCliCode {
    LOR_CLI_OK = 0,
    LOR_CLI_HELP_REQUESTED,
    LOR_CLI_INVALID_STATE,
    LOR_CLI_UNKNOWN_OPTION,
    LOR_CLI_MISSING_VALUE,
    LOR_CLI_UNEXPECTED_VALUE,
    LOR_CLI_INVALID_VALUE,
    LOR_CLI_DUPLICATE_OPTION,
    LOR_CLI_MISSING_REQUIRED_OPTION,
    LOR_CLI_MISSING_POSITIONAL,
    LOR_CLI_UNEXPECTED_POSITIONAL,
    LOR_CLI_OUT_OF_MEMORY
} LorCliCode;

typedef struct LorCliResult {
    LorCliCode code;
    int argv_index;
    LorStringView token;
    const char *name;
    char short_name;
} LorCliResult;

#define LOR_CLI_RESULT_INIT {LOR_CLI_OK, -1, LOR_STRING_VIEW_INIT, NULL, 0}

/* Initializes parser metadata without allocating.

   `program_name`, `description`, option names, help strings, and value names
   are borrowed and must remain valid through parsing and help generation. */
void lor_cli_init(LorCli *cli, const char *program_name, const char *description);

// Releases registered definitions and generated default text.
void lor_cli_deinit(LorCli *cli);

// Returns a stable lowercase name for `code`, or "unknown".
const char *lor_cli_code_name(LorCliCode code);

// Registers common optional values with concise defaults.
LorStatus lor_cli_add_flag(LorCli *cli, char short_name, const char *long_name,
                           bool *destination, const char *help);
LorStatus lor_cli_add_count(LorCli *cli, char short_name, const char *long_name,
                            size_t *destination, const char *help);
LorStatus lor_cli_add_string(LorCli *cli, char short_name, const char *long_name,
                             LorStringView *destination, const char *help);
LorStatus lor_cli_add_int(LorCli *cli, char short_name, const char *long_name,
                          int64_t *destination, const char *help);
LorStatus lor_cli_add_double(LorCli *cli, char short_name, const char *long_name,
                             double *destination, const char *help);

// Descriptor forms support required, repeatable, metavar, and default display.
LorStatus lor_cli_add_flag_option(LorCli *cli, LorCliOption option,
                                  bool *destination);
LorStatus lor_cli_add_count_option(LorCli *cli, LorCliOption option,
                                   size_t *destination);
LorStatus lor_cli_add_string_option(LorCli *cli, LorCliOption option,
                                    LorStringView *destination);
LorStatus lor_cli_add_int_option(LorCli *cli, LorCliOption option,
                                 int64_t *destination);
LorStatus lor_cli_add_double_option(LorCli *cli, LorCliOption option,
                                    double *destination);

/* Repeated option forms append each parsed value to a dynamic array.

   Destination handles must start as `NULL`. The caller owns and deinitializes
   the resulting array with `lor_array_deinit`. */
LorStatus lor_cli_add_strings_option(LorCli *cli, LorCliOption option,
                                     LorStringView **destination);
LorStatus lor_cli_add_ints_option(LorCli *cli, LorCliOption option,
                                  int64_t **destination);
LorStatus lor_cli_add_doubles_option(LorCli *cli, LorCliOption option,
                                     double **destination);

// Registers ordered scalar positional arguments.
LorStatus lor_cli_add_positional_string(LorCli *cli, LorCliPositional positional,
                                        LorStringView *destination);
LorStatus lor_cli_add_positional_int(LorCli *cli, LorCliPositional positional,
                                     int64_t *destination);
LorStatus lor_cli_add_positional_double(LorCli *cli, LorCliPositional positional,
                                        double *destination);

/* Registers a trailing list that consumes all remaining positionals.

   It must be the final positional definition. The destination is a dynamic
   `LorStringView` array owned by the caller. */
LorStatus lor_cli_add_positionals(LorCli *cli, const char *name,
                                  LorStringView **destination, const char *help);

/* Parses `argv`.

   `argv[0]` supplies the program name when none was configured. Strings are
   borrowed views into `argv`. The parser recognizes `-h` and `--help`
   automatically and returns `LOR_CLI_HELP_REQUESTED` without printing or
   validating required arguments. A parser may be parsed once.

   Parsing writes destinations as values are accepted. On a later parse error,
   already accepted scalar values and repeated-array elements remain written. */
LorCliResult lor_cli_parse(LorCli *cli, int argc, char *argv[]);

// Writes a descriptive parse error without adding help text.
int lor_cli_fprint_error(FILE *out, LorCliResult result);

// Writes generated usage and help text. Returns non-zero on complete output.
int lor_cli_fprint_help(const LorCli *cli, FILE *out);

// stdout form of `lor_cli_fprint_help`.
int lor_cli_print_help(const LorCli *cli);

/* Scope-exit cleanup for parsers on compilers with cleanup attributes.

   Unsupported compilers leave `LOR_AUTO_CLI` empty, so explicit
   `lor_cli_deinit` remains required for portable ownership paths. */
#if LOR_HAS_CLEANUP_ATTRIBUTE
static inline void __attribute__((unused)) lor_cli_cleanup_(LorCli *cli) {
    lor_cli_deinit(cli);
}
#define LOR_AUTO_CLI __attribute__((cleanup(lor_cli_cleanup_)))
#else
#define LOR_AUTO_CLI
#endif

#ifdef __cplusplus
}
#endif

#endif
