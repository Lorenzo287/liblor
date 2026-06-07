# Command-Line Parsing

`lor/cli.h` provides a function-based command-line parser with generated help,
checked numeric conversion, and no global parser state. It takes inspiration
from the metadata-driven interfaces of Python's `argparse` and Go's `flag`
package without copying their APIs or hiding declarations in large macros.

## Basic Use

Create a parser, register destinations, then parse once:

```c
LorCli cli = LOR_CLI_INIT;
bool verbose = false;
int64_t jobs = 1;
LorStringView input = LOR_STRING_VIEW_INIT;

lor_cli_init(&cli, NULL, "Process one input file.");
lor_cli_add_flag(&cli, 'v', "verbose", &verbose, "Enable details");
lor_cli_add_int(&cli, 'j', "jobs", &jobs, "Worker count");

LorCliPositional positional = LOR_CLI_POSITIONAL_INIT;
positional.name = "input";
lor_cli_add_positional_string(&cli, positional, &input);

LorCliResult result = lor_cli_parse(&cli, argc, argv);
if (result.code == LOR_CLI_HELP_REQUESTED) {
    lor_cli_print_help(&cli);
} else if (result.code != LOR_CLI_OK) {
    lor_cli_fprint_error(stderr, result);
}

lor_cli_deinit(&cli);
```

The parser automatically reserves `-h` and `--help`. Parsing returns
`LOR_CLI_HELP_REQUESTED`; it does not print help, terminate the process, or
validate required arguments. The application decides the output stream and
exit code.

## Registration

The concise option functions cover common cases:

- `lor_cli_add_flag`: sets a `bool` to `true`.
- `lor_cli_add_count`: increments a `size_t`; clustered `-vvv` is supported.
- `lor_cli_add_string`: writes a borrowed `LorStringView`.
- `lor_cli_add_int`: parses a decimal `int64_t`.
- `lor_cli_add_double`: parses a `double`.

The `_option` forms accept `LorCliOption` metadata for required options,
custom value names, repeatability, and default-value display. Initialize the
descriptor with `LOR_CLI_OPTION_INIT`, then set the fields that differ.

Repeated string, integer, and double options append to caller-owned liblor
dynamic arrays. Their destination handles must start as `NULL`, and the caller
must release them with `lor_array_deinit`.

Positionals are consumed in registration order. Required positionals cannot
follow optional positionals. `lor_cli_add_positionals` registers one final
dynamic array that consumes all remaining positional values.

## Accepted Syntax

The parser accepts:

- `--output=file` and `--output file`;
- `-oFILE`, `-o=FILE`, and `-o FILE`;
- clustered flags and counters such as `-qvv`;
- options interspersed with positional arguments;
- `--` to stop option parsing;
- negative numeric values when the next positional expects an integer or
  double.

Options reject duplicate occurrences unless they are repeatable. Counting and
typed repeated options are always repeatable.

## Ownership And Failure

Parser metadata strings are borrowed and must remain valid through parsing and
help generation. Parsed strings are views into `argv`; copy them into a
`LorString` if they must outlive that storage.

`LorCli` owns its registered definitions and generated default text.
`lor_cli_deinit` releases that storage and resets the parser. On GCC and Clang,
`LOR_AUTO_CLI` provides scope-exit cleanup.

Registration returns `LorStatus`. Parsing returns `LorCliResult`, because CLI
errors need the failure category, argument index, offending token, and option
or positional name. Parsing writes destinations incrementally; values accepted
before a later error remain written.

See `examples/cli.c` for a complete program.
