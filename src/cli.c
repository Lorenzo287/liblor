// SPDX-License-Identifier: MIT

#include "lor/cli.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum LorCliValueKind {
    LOR_CLI_VALUE_FLAG,
    LOR_CLI_VALUE_COUNT,
    LOR_CLI_VALUE_STRING,
    LOR_CLI_VALUE_INT,
    LOR_CLI_VALUE_DOUBLE,
    LOR_CLI_VALUE_STRINGS,
    LOR_CLI_VALUE_INTS,
    LOR_CLI_VALUE_DOUBLES,
    LOR_CLI_VALUE_POSITIONALS
} LorCliValueKind;

typedef struct LorCliOptionInternal {
    LorCliOption spec;
    LorCliValueKind kind;
    void *destination;
    size_t occurrences;
    LorString default_text;
} LorCliOptionInternal;

typedef struct LorCliPositionalInternal {
    LorCliPositional spec;
    LorCliValueKind kind;
    void *destination;
    size_t occurrences;
} LorCliPositionalInternal;

static LorCliOptionInternal *lor_cli__option_data(const LorCli *cli) {
    return (LorCliOptionInternal *)cli->impl_options;
}

static LorCliPositionalInternal *lor_cli__positional_data(const LorCli *cli) {
    return (LorCliPositionalInternal *)cli->impl_positionals;
}

static LorCliResult lor_cli__result(LorCliCode code, int argv_index,
                                    const char *token, const char *name) {
    LorCliResult result = LOR_CLI_RESULT_INIT;
    result.code = code;
    result.argv_index = argv_index;
    result.token = lor_sv_from_cstr(token);
    result.name = name;
    return result;
}

static LorCliResult lor_cli__option_result(LorCliCode code, int argv_index,
                                           const char *token,
                                           const LorCliOptionInternal *option) {
    LorCliResult result =
        lor_cli__result(code, argv_index, token, option->spec.long_name);
    result.short_name = option->spec.short_name;
    return result;
}

void lor_cli_init(LorCli *cli, const char *program_name, const char *description) {
    if (cli == NULL) return;
    *cli = (LorCli)LOR_CLI_INIT;
    cli->program_name = program_name;
    cli->description = description;
}

void lor_cli_deinit(LorCli *cli) {
    if (cli == NULL) return;

    LorCliOptionInternal *options = lor_cli__option_data(cli);
    for (size_t i = 0; i < lor_array_size(options); ++i)
        lor_string_deinit(&options[i].default_text);
    lor_array_deinit(&options);

    LorCliPositionalInternal *positionals = lor_cli__positional_data(cli);
    lor_array_deinit(&positionals);
    *cli = (LorCli)LOR_CLI_INIT;
}

const char *lor_cli_code_name(LorCliCode code) {
    switch (code) {
    case LOR_CLI_OK:
        return "ok";
    case LOR_CLI_HELP_REQUESTED:
        return "help requested";
    case LOR_CLI_INVALID_STATE:
        return "invalid state";
    case LOR_CLI_UNKNOWN_OPTION:
        return "unknown option";
    case LOR_CLI_MISSING_VALUE:
        return "missing value";
    case LOR_CLI_UNEXPECTED_VALUE:
        return "unexpected value";
    case LOR_CLI_INVALID_VALUE:
        return "invalid value";
    case LOR_CLI_DUPLICATE_OPTION:
        return "duplicate option";
    case LOR_CLI_MISSING_REQUIRED_OPTION:
        return "missing required option";
    case LOR_CLI_MISSING_POSITIONAL:
        return "missing positional";
    case LOR_CLI_UNEXPECTED_POSITIONAL:
        return "unexpected positional";
    case LOR_CLI_OUT_OF_MEMORY:
        return "out of memory";
    }
    return "unknown";
}

static int lor_cli__long_name_valid(const char *name) {
    if (name == NULL || name[0] == '\0' || name[0] == '-') return 0;
    for (const char *cursor = name; *cursor != '\0'; ++cursor) {
        if (*cursor == '=' || isspace((unsigned char)*cursor)) return 0;
    }
    return strcmp(name, "help") != 0;
}

static int lor_cli__short_name_valid(char name) {
    return name == '\0' || (name != 'h' && name != '-' && name != '=' &&
                            !isspace((unsigned char)name));
}

static LorCliOptionInternal *lor_cli__find_short(const LorCli *cli, char name) {
    LorCliOptionInternal *options = lor_cli__option_data(cli);
    for (size_t i = 0; i < lor_array_size(options); ++i) {
        if (options[i].spec.short_name == name) return &options[i];
    }
    return NULL;
}

static LorCliOptionInternal *lor_cli__find_long_parts(const LorCli *cli,
                                                      const char *name,
                                                      size_t size) {
    LorCliOptionInternal *options = lor_cli__option_data(cli);
    for (size_t i = 0; i < lor_array_size(options); ++i) {
        const char *candidate = options[i].spec.long_name;
        if (candidate != NULL && strlen(candidate) == size &&
            memcmp(candidate, name, size) == 0)
            return &options[i];
    }
    return NULL;
}

static int lor_cli__option_conflicts(const LorCli *cli, LorCliOption option) {
    if (option.short_name != '\0' &&
        lor_cli__find_short(cli, option.short_name) != NULL)
        return 1;
    return option.long_name != NULL &&
           lor_cli__find_long_parts(cli, option.long_name,
                                    strlen(option.long_name)) != NULL;
}

static const char *lor_cli__default_value_name(LorCliValueKind kind) {
    switch (kind) {
    case LOR_CLI_VALUE_STRING:
    case LOR_CLI_VALUE_STRINGS:
        return "TEXT";
    case LOR_CLI_VALUE_INT:
    case LOR_CLI_VALUE_INTS:
        return "INT";
    case LOR_CLI_VALUE_DOUBLE:
    case LOR_CLI_VALUE_DOUBLES:
        return "NUMBER";
    default:
        return NULL;
    }
}

static LorStatus lor_cli__capture_default(LorCliOptionInternal *option) {
    if (!option->spec.show_default || option->kind == LOR_CLI_VALUE_FLAG ||
        option->kind == LOR_CLI_VALUE_COUNT ||
        option->kind == LOR_CLI_VALUE_STRINGS ||
        option->kind == LOR_CLI_VALUE_INTS || option->kind == LOR_CLI_VALUE_DOUBLES)
        return LOR_STATUS_OK;

    char buffer[128];
    switch (option->kind) {
    case LOR_CLI_VALUE_STRING: {
        LorStringView value = *(LorStringView *)option->destination;
        if (!lor_sv_is_valid(value) || value.size == 0) return LOR_STATUS_OK;
        return lor_string_assign(&option->default_text, value);
    }
    case LOR_CLI_VALUE_INT:
        snprintf(buffer, sizeof(buffer), "%" PRId64,
                 *(int64_t *)option->destination);
        return lor_string_assign_cstr(&option->default_text, buffer);
    case LOR_CLI_VALUE_DOUBLE:
        snprintf(buffer, sizeof(buffer), "%.17g", *(double *)option->destination);
        return lor_string_assign_cstr(&option->default_text, buffer);
    default:
        return LOR_STATUS_OK;
    }
}

static LorStatus lor_cli__add_option(LorCli *cli, LorCliOption option,
                                     LorCliValueKind kind, void *destination) {
    if (cli == NULL || destination == NULL || cli->parsed ||
        (option.short_name == '\0' && option.long_name == NULL) ||
        !lor_cli__short_name_valid(option.short_name) ||
        (option.long_name != NULL && !lor_cli__long_name_valid(option.long_name)) ||
        lor_cli__option_conflicts(cli, option))
        return LOR_STATUS_INVALID_ARGUMENT;

    if (kind == LOR_CLI_VALUE_FLAG || kind == LOR_CLI_VALUE_COUNT) {
        option.value_name = NULL;
        option.show_default = 0;
        if (kind == LOR_CLI_VALUE_COUNT) option.repeatable = 1;
    } else if (option.value_name == NULL) {
        option.value_name = lor_cli__default_value_name(kind);
    }

    if (kind == LOR_CLI_VALUE_STRINGS || kind == LOR_CLI_VALUE_INTS ||
        kind == LOR_CLI_VALUE_DOUBLES) {
        option.repeatable = 1;
        option.show_default = 0;
    }

    LorCliOptionInternal internal = {
        .spec = option,
        .kind = kind,
        .destination = destination,
        .default_text = LOR_STRING_INIT,
    };
    LorStatus status = lor_cli__capture_default(&internal);
    if (status != LOR_STATUS_OK) return status;

    LorCliOptionInternal *options = lor_cli__option_data(cli);
    status = lor_array_push(options, internal);
    cli->impl_options = options;
    if (status != LOR_STATUS_OK) lor_string_deinit(&internal.default_text);
    return status;
}

static LorCliOption lor_cli__simple_option(char short_name, const char *long_name,
                                           const char *help) {
    LorCliOption option = LOR_CLI_OPTION_INIT;
    option.short_name = short_name;
    option.long_name = long_name;
    option.help = help;
    return option;
}

LorStatus lor_cli_add_flag(LorCli *cli, char short_name, const char *long_name,
                           bool *destination, const char *help) {
    return lor_cli_add_flag_option(
        cli, lor_cli__simple_option(short_name, long_name, help), destination);
}

LorStatus lor_cli_add_count(LorCli *cli, char short_name, const char *long_name,
                            size_t *destination, const char *help) {
    LorCliOption option = lor_cli__simple_option(short_name, long_name, help);
    option.repeatable = 1;
    return lor_cli_add_count_option(cli, option, destination);
}

LorStatus lor_cli_add_string(LorCli *cli, char short_name, const char *long_name,
                             LorStringView *destination, const char *help) {
    return lor_cli_add_string_option(
        cli, lor_cli__simple_option(short_name, long_name, help), destination);
}

LorStatus lor_cli_add_int(LorCli *cli, char short_name, const char *long_name,
                          int64_t *destination, const char *help) {
    return lor_cli_add_int_option(
        cli, lor_cli__simple_option(short_name, long_name, help), destination);
}

LorStatus lor_cli_add_double(LorCli *cli, char short_name, const char *long_name,
                             double *destination, const char *help) {
    return lor_cli_add_double_option(
        cli, lor_cli__simple_option(short_name, long_name, help), destination);
}

LorStatus lor_cli_add_flag_option(LorCli *cli, LorCliOption option,
                                  bool *destination) {
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_FLAG, destination);
}

LorStatus lor_cli_add_count_option(LorCli *cli, LorCliOption option,
                                   size_t *destination) {
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_COUNT, destination);
}

LorStatus lor_cli_add_string_option(LorCli *cli, LorCliOption option,
                                    LorStringView *destination) {
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_STRING, destination);
}

LorStatus lor_cli_add_int_option(LorCli *cli, LorCliOption option,
                                 int64_t *destination) {
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_INT, destination);
}

LorStatus lor_cli_add_double_option(LorCli *cli, LorCliOption option,
                                    double *destination) {
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_DOUBLE, destination);
}

LorStatus lor_cli_add_strings_option(LorCli *cli, LorCliOption option,
                                     LorStringView **destination) {
    if (destination == NULL || *destination != NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_STRINGS, destination);
}

LorStatus lor_cli_add_ints_option(LorCli *cli, LorCliOption option,
                                  int64_t **destination) {
    if (destination == NULL || *destination != NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_INTS, destination);
}

LorStatus lor_cli_add_doubles_option(LorCli *cli, LorCliOption option,
                                     double **destination) {
    if (destination == NULL || *destination != NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    return lor_cli__add_option(cli, option, LOR_CLI_VALUE_DOUBLES, destination);
}

static int lor_cli__has_rest(const LorCli *cli) {
    LorCliPositionalInternal *items = lor_cli__positional_data(cli);
    size_t size = lor_array_size(items);
    return size != 0 && items[size - 1u].kind == LOR_CLI_VALUE_POSITIONALS;
}

static LorStatus lor_cli__add_positional(LorCli *cli, LorCliPositional positional,
                                         LorCliValueKind kind, void *destination) {
    if (cli == NULL || destination == NULL || cli->parsed ||
        positional.name == NULL || positional.name[0] == '\0' ||
        lor_cli__has_rest(cli))
        return LOR_STATUS_INVALID_ARGUMENT;

    LorCliPositionalInternal *items = lor_cli__positional_data(cli);
    if (positional.required != 0 && lor_array_size(items) != 0 &&
        items[lor_array_size(items) - 1u].spec.required == 0)
        return LOR_STATUS_INVALID_ARGUMENT;

    LorCliPositionalInternal internal = {
        .spec = positional,
        .kind = kind,
        .destination = destination,
    };
    LorStatus status = lor_array_push(items, internal);
    cli->impl_positionals = items;
    return status;
}

LorStatus lor_cli_add_positional_string(LorCli *cli, LorCliPositional positional,
                                        LorStringView *destination) {
    return lor_cli__add_positional(cli, positional, LOR_CLI_VALUE_STRING,
                                   destination);
}

LorStatus lor_cli_add_positional_int(LorCli *cli, LorCliPositional positional,
                                     int64_t *destination) {
    return lor_cli__add_positional(cli, positional, LOR_CLI_VALUE_INT, destination);
}

LorStatus lor_cli_add_positional_double(LorCli *cli, LorCliPositional positional,
                                        double *destination) {
    return lor_cli__add_positional(cli, positional, LOR_CLI_VALUE_DOUBLE,
                                   destination);
}

LorStatus lor_cli_add_positionals(LorCli *cli, const char *name,
                                  LorStringView **destination, const char *help) {
    if (destination == NULL || *destination != NULL)
        return LOR_STATUS_INVALID_ARGUMENT;
    LorCliPositional positional = {
        .name = name,
        .help = help,
        .required = 0,
    };
    return lor_cli__add_positional(cli, positional, LOR_CLI_VALUE_POSITIONALS,
                                   destination);
}

static int lor_cli__parse_int(const char *text, int64_t *destination) {
    if (text == NULL || text[0] == '\0') return 0;
    errno = 0;
    char *end = NULL;
    long long value = strtoll(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') return 0;
    if (value < INT64_MIN || value > INT64_MAX) return 0;
    *destination = (int64_t)value;
    return 1;
}

static int lor_cli__parse_double(const char *text, double *destination) {
    if (text == NULL || text[0] == '\0') return 0;
    errno = 0;
    char *end = NULL;
    double value = strtod(text, &end);
    if (errno == ERANGE || end == text || *end != '\0') return 0;
    *destination = value;
    return 1;
}

static LorCliCode lor_cli__assign_value(LorCliValueKind kind, void *destination,
                                        const char *text) {
    LorStatus status = LOR_STATUS_OK;
    switch (kind) {
    case LOR_CLI_VALUE_STRING:
        *(LorStringView *)destination = lor_sv_from_cstr(text);
        return LOR_CLI_OK;
    case LOR_CLI_VALUE_INT:
        return lor_cli__parse_int(text, (int64_t *)destination)
                   ? LOR_CLI_OK
                   : LOR_CLI_INVALID_VALUE;
    case LOR_CLI_VALUE_DOUBLE:
        return lor_cli__parse_double(text, (double *)destination)
                   ? LOR_CLI_OK
                   : LOR_CLI_INVALID_VALUE;
    case LOR_CLI_VALUE_STRINGS: {
        LorStringView value = lor_sv_from_cstr(text);
        status = lor_array_push(*(LorStringView **)destination, value);
        break;
    }
    case LOR_CLI_VALUE_INTS: {
        int64_t value = 0;
        if (!lor_cli__parse_int(text, &value)) return LOR_CLI_INVALID_VALUE;
        status = lor_array_push(*(int64_t **)destination, value);
        break;
    }
    case LOR_CLI_VALUE_DOUBLES: {
        double value = 0;
        if (!lor_cli__parse_double(text, &value)) return LOR_CLI_INVALID_VALUE;
        status = lor_array_push(*(double **)destination, value);
        break;
    }
    case LOR_CLI_VALUE_POSITIONALS: {
        LorStringView value = lor_sv_from_cstr(text);
        status = lor_array_push(*(LorStringView **)destination, value);
        break;
    }
    default:
        return LOR_CLI_INVALID_STATE;
    }
    return status == LOR_STATUS_OK ? LOR_CLI_OK : LOR_CLI_OUT_OF_MEMORY;
}

static LorCliResult lor_cli__apply_option(LorCliOptionInternal *option,
                                          const char *value, int argv_index,
                                          const char *token) {
    if (option->occurrences != 0 && !option->spec.repeatable)
        return lor_cli__option_result(LOR_CLI_DUPLICATE_OPTION, argv_index, token,
                                      option);

    if (option->kind == LOR_CLI_VALUE_FLAG) {
        if (value != NULL)
            return lor_cli__option_result(LOR_CLI_UNEXPECTED_VALUE, argv_index,
                                          token, option);
        *(bool *)option->destination = true;
    } else if (option->kind == LOR_CLI_VALUE_COUNT) {
        if (value != NULL)
            return lor_cli__option_result(LOR_CLI_UNEXPECTED_VALUE, argv_index,
                                          token, option);
        size_t *count = (size_t *)option->destination;
        if (*count == SIZE_MAX)
            return lor_cli__option_result(LOR_CLI_INVALID_VALUE, argv_index, token,
                                          option);
        *count += 1u;
    } else {
        if (value == NULL)
            return lor_cli__option_result(LOR_CLI_MISSING_VALUE, argv_index, token,
                                          option);
        LorCliCode code =
            lor_cli__assign_value(option->kind, option->destination, value);
        if (code != LOR_CLI_OK) {
            LorCliResult result =
                lor_cli__option_result(code, argv_index, value, option);
            return result;
        }
    }

    option->occurrences += 1u;
    return lor_cli__option_result(LOR_CLI_OK, argv_index, token, option);
}

static int lor_cli__positional_numeric(const LorCli *cli, size_t positional_index,
                                       const char *token) {
    LorCliPositionalInternal *items = lor_cli__positional_data(cli);
    if (positional_index >= lor_array_size(items)) return 0;
    int64_t int_value = 0;
    double double_value = 0;
    if (items[positional_index].kind == LOR_CLI_VALUE_INT)
        return lor_cli__parse_int(token, &int_value);
    if (items[positional_index].kind == LOR_CLI_VALUE_DOUBLE)
        return lor_cli__parse_double(token, &double_value);
    return 0;
}

static LorCliResult lor_cli__consume_positional(LorCli *cli,
                                                size_t *positional_index,
                                                const char *token, int argv_index) {
    LorCliPositionalInternal *items = lor_cli__positional_data(cli);
    size_t count = lor_array_size(items);
    if (*positional_index >= count)
        return lor_cli__result(LOR_CLI_UNEXPECTED_POSITIONAL, argv_index, token,
                               NULL);

    LorCliPositionalInternal *item = &items[*positional_index];
    LorCliCode code = lor_cli__assign_value(item->kind, item->destination, token);
    if (code != LOR_CLI_OK)
        return lor_cli__result(code, argv_index, token, item->spec.name);

    item->occurrences += 1u;
    if (item->kind != LOR_CLI_VALUE_POSITIONALS) *positional_index += 1u;
    return lor_cli__result(LOR_CLI_OK, argv_index, token, item->spec.name);
}

LorCliResult lor_cli_parse(LorCli *cli, int argc, char *argv[]) {
    if (cli == NULL || cli->parsed || argc < 0 || (argc != 0 && argv == NULL))
        return lor_cli__result(LOR_CLI_INVALID_STATE, -1, NULL, NULL);

    cli->parsed = 1;
    if (cli->program_name == NULL && argc > 0) cli->program_name = argv[0];

    size_t positional_index = 0;
    int options_enabled = 1;
    for (int i = argc > 0 ? 1 : 0; i < argc; ++i) {
        const char *token = argv[i];
        if (token == NULL)
            return lor_cli__result(LOR_CLI_INVALID_STATE, i, NULL, NULL);

        if (options_enabled && strcmp(token, "--") == 0) {
            options_enabled = 0;
            continue;
        }
        if (options_enabled &&
            (strcmp(token, "-h") == 0 || strcmp(token, "--help") == 0))
            return lor_cli__result(LOR_CLI_HELP_REQUESTED, i, token, "help");

        if (options_enabled && token[0] == '-' && token[1] != '\0' &&
            lor_cli__positional_numeric(cli, positional_index, token)) {
            LorCliResult result =
                lor_cli__consume_positional(cli, &positional_index, token, i);
            if (result.code != LOR_CLI_OK) return result;
            continue;
        }

        if (options_enabled && token[0] == '-' && token[1] == '-') {
            int option_index = i;
            const char *name = token + 2;
            const char *equals = strchr(name, '=');
            size_t name_size =
                equals != NULL ? (size_t)(equals - name) : strlen(name);
            LorCliOptionInternal *option =
                lor_cli__find_long_parts(cli, name, name_size);
            if (option == NULL)
                return lor_cli__result(LOR_CLI_UNKNOWN_OPTION, i, token, NULL);

            const char *value = equals != NULL ? equals + 1 : NULL;
            if (value == NULL && option->kind != LOR_CLI_VALUE_FLAG &&
                option->kind != LOR_CLI_VALUE_COUNT) {
                if (i + 1 >= argc)
                    return lor_cli__option_result(LOR_CLI_MISSING_VALUE,
                                                  option_index, token, option);
                value = argv[++i];
            }
            LorCliResult result =
                lor_cli__apply_option(option, value, option_index, token);
            if (result.code != LOR_CLI_OK) return result;
            continue;
        }

        if (options_enabled && token[0] == '-' && token[1] != '\0') {
            int option_index = i;
            const char *cursor = token + 1;
            while (*cursor != '\0') {
                char name = *cursor++;
                if (name == 'h')
                    return lor_cli__result(LOR_CLI_HELP_REQUESTED, i, token, "help");
                LorCliOptionInternal *option = lor_cli__find_short(cli, name);
                if (option == NULL)
                    return lor_cli__result(LOR_CLI_UNKNOWN_OPTION, i, token, NULL);

                const char *value = NULL;
                if (option->kind != LOR_CLI_VALUE_FLAG &&
                    option->kind != LOR_CLI_VALUE_COUNT) {
                    if (*cursor == '=') cursor += 1;
                    if (*cursor != '\0') {
                        value = cursor;
                    } else {
                        if (i + 1 >= argc)
                            return lor_cli__option_result(
                                LOR_CLI_MISSING_VALUE, option_index, token, option);
                        value = argv[++i];
                    }
                    cursor += strlen(cursor);
                }

                LorCliResult result =
                    lor_cli__apply_option(option, value, option_index, token);
                if (result.code != LOR_CLI_OK) return result;
            }
            continue;
        }

        LorCliResult result =
            lor_cli__consume_positional(cli, &positional_index, token, i);
        if (result.code != LOR_CLI_OK) return result;
    }

    LorCliOptionInternal *options = lor_cli__option_data(cli);
    for (size_t i = 0; i < lor_array_size(options); ++i) {
        if (options[i].spec.required && options[i].occurrences == 0)
            return lor_cli__option_result(LOR_CLI_MISSING_REQUIRED_OPTION, -1, NULL,
                                          &options[i]);
    }

    LorCliPositionalInternal *positionals = lor_cli__positional_data(cli);
    for (size_t i = 0; i < lor_array_size(positionals); ++i) {
        if (positionals[i].spec.required && positionals[i].occurrences == 0)
            return lor_cli__result(LOR_CLI_MISSING_POSITIONAL, -1, NULL,
                                   positionals[i].spec.name);
    }

    return lor_cli__result(LOR_CLI_OK, -1, NULL, NULL);
}

static int lor_cli__print_view(FILE *out, LorStringView view) {
    return lor_sv_fprint(out, view);
}

static int lor_cli__fprint_option_name(FILE *out, LorCliResult result) {
    if (result.name != NULL) return fprintf(out, "--%s", result.name) >= 0;
    if (result.short_name != '\0')
        return fprintf(out, "-%c", result.short_name) >= 0;
    return lor_cli__print_view(out, result.token);
}

int lor_cli_fprint_error(FILE *out, LorCliResult result) {
    if (out == NULL || result.code == LOR_CLI_OK ||
        result.code == LOR_CLI_HELP_REQUESTED)
        return 0;

    if (fputs("error: ", out) < 0) return 0;
    switch (result.code) {
    case LOR_CLI_UNKNOWN_OPTION:
        if (fputs("unknown option '", out) < 0 ||
            !lor_cli__print_view(out, result.token) || fputs("'\n", out) < 0)
            return 0;
        break;
    case LOR_CLI_MISSING_VALUE:
        if (fputs("option requires a value: '", out) < 0 ||
            !lor_cli__print_view(out, result.token) || fputs("'\n", out) < 0)
            return 0;
        break;
    case LOR_CLI_UNEXPECTED_VALUE:
        if (fputs("option does not accept a value: '", out) < 0 ||
            !lor_cli__print_view(out, result.token) || fputs("'\n", out) < 0)
            return 0;
        break;
    case LOR_CLI_INVALID_VALUE:
        if (fputs("invalid value '", out) < 0 ||
            !lor_cli__print_view(out, result.token) || fputs("'", out) < 0)
            return 0;
        if (result.name != NULL && fprintf(out, " for %s", result.name) < 0)
            return 0;
        if (fputc('\n', out) == EOF) return 0;
        break;
    case LOR_CLI_DUPLICATE_OPTION:
        if (fputs("option specified more than once: ", out) < 0 ||
            !lor_cli__fprint_option_name(out, result) || fputc('\n', out) == EOF)
            return 0;
        break;
    case LOR_CLI_MISSING_REQUIRED_OPTION:
        if (fputs("missing required option: ", out) < 0 ||
            !lor_cli__fprint_option_name(out, result) || fputc('\n', out) == EOF)
            return 0;
        break;
    case LOR_CLI_MISSING_POSITIONAL:
        if (fprintf(out, "missing positional argument: %s\n",
                    result.name != NULL ? result.name : "") < 0)
            return 0;
        break;
    case LOR_CLI_UNEXPECTED_POSITIONAL:
        if (fputs("unexpected positional argument: '", out) < 0 ||
            !lor_cli__print_view(out, result.token) || fputs("'\n", out) < 0)
            return 0;
        break;
    default:
        if (fprintf(out, "%s\n", lor_cli_code_name(result.code)) < 0) return 0;
        break;
    }
    return 1;
}

static size_t lor_cli__option_label_size(const LorCliOptionInternal *option) {
    size_t size = 0;
    if (option->spec.short_name != '\0') size += 2u;
    if (option->spec.short_name != '\0' && option->spec.long_name != NULL)
        size += 2u;
    if (option->spec.long_name != NULL) size += 2u + strlen(option->spec.long_name);
    if (option->spec.value_name != NULL)
        size += 1u + strlen(option->spec.value_name);
    return size;
}

static int lor_cli__fprint_option_label(FILE *out,
                                        const LorCliOptionInternal *option) {
    if (option->spec.short_name != '\0' &&
        fprintf(out, "-%c", option->spec.short_name) < 0)
        return 0;
    if (option->spec.short_name != '\0' && option->spec.long_name != NULL &&
        fputs(", ", out) < 0)
        return 0;
    if (option->spec.long_name != NULL &&
        fprintf(out, "--%s", option->spec.long_name) < 0)
        return 0;
    if (option->spec.value_name != NULL &&
        fprintf(out, " %s", option->spec.value_name) < 0)
        return 0;
    return 1;
}

int lor_cli_fprint_help(const LorCli *cli, FILE *out) {
    if (cli == NULL || out == NULL) return 0;
    const char *program = cli->program_name != NULL ? cli->program_name : "program";

    if (fprintf(out, "Usage: %s", program) < 0) return 0;
    if (fputs(" [options]", out) < 0) return 0;

    LorCliPositionalInternal *positionals = lor_cli__positional_data(cli);
    for (size_t i = 0; i < lor_array_size(positionals); ++i) {
        LorCliPositionalInternal *item = &positionals[i];
        if (item->kind == LOR_CLI_VALUE_POSITIONALS) {
            if (fprintf(out, " [%s...]", item->spec.name) < 0) return 0;
        } else if (item->spec.required) {
            if (fprintf(out, " <%s>", item->spec.name) < 0) return 0;
        } else if (fprintf(out, " [%s]", item->spec.name) < 0) {
            return 0;
        }
    }
    if (fputc('\n', out) == EOF) return 0;

    if (cli->description != NULL && cli->description[0] != '\0') {
        if (fprintf(out, "\n%s\n", cli->description) < 0) return 0;
    }

    LorCliOptionInternal *options = lor_cli__option_data(cli);
    size_t width = strlen("-h, --help");
    for (size_t i = 0; i < lor_array_size(options); ++i) {
        size_t size = lor_cli__option_label_size(&options[i]);
        if (size > width) width = size;
    }
    if (width > 40u) width = 40u;

    if (fputs("\nOptions:\n", out) < 0) return 0;
    for (size_t i = 0; i < lor_array_size(options); ++i) {
        size_t label_size = lor_cli__option_label_size(&options[i]);
        size_t padding = label_size < width ? width - label_size : 0;
        if (fputs("  ", out) < 0 ||
            !lor_cli__fprint_option_label(out, &options[i]) ||
            fprintf(out, "%*s  %s", (int)padding, "",
                    options[i].spec.help != NULL ? options[i].spec.help : "") < 0)
            return 0;
        if (options[i].spec.required && fputs(" (required)", out) < 0) return 0;
        if (options[i].default_text != NULL &&
            fprintf(out, " (default: %s)",
                    lor_string_cstr(options[i].default_text)) < 0)
            return 0;
        if (fputc('\n', out) == EOF) return 0;
    }

    if (fprintf(out, "  %-*s  Show this help message\n", (int)width, "-h, --help") <
        0)
        return 0;

    if (lor_array_size(positionals) != 0) {
        size_t positional_width = 0;
        for (size_t i = 0; i < lor_array_size(positionals); ++i) {
            size_t size = strlen(positionals[i].spec.name);
            if (size > positional_width) positional_width = size;
        }
        if (positional_width > 40u) positional_width = 40u;
        if (fputs("\nArguments:\n", out) < 0) return 0;
        for (size_t i = 0; i < lor_array_size(positionals); ++i) {
            if (fprintf(out, "  %-*s  %s\n", (int)positional_width,
                        positionals[i].spec.name,
                        positionals[i].spec.help != NULL ? positionals[i].spec.help
                                                         : "") < 0)
                return 0;
        }
    }
    return 1;
}

int lor_cli_print_help(const LorCli *cli) {
    return lor_cli_fprint_help(cli, stdout);
}
