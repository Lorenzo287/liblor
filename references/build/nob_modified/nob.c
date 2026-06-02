#define NOBDEF static inline
#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"
#define INCLUDE_FOLDER "include/"

int main(int argc, char **argv) {
    GO_REBUILD_URSELF(argc, argv);
    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    Cmd cmd = {0};
    cc(&cmd);
    cc_flags(&cmd);
#ifndef _MSC_VER
    cmd_append(&cmd, "-O2");
    cmd_append(&cmd, "-I" INCLUDE_FOLDER);
#else
    cmd_append(&cmd, "/O2");
    cmd_append(&cmd, "/I" INCLUDE_FOLDER);
#endif

    Cmd targets = {0};
    cc_inputs(&targets, SRC_FOLDER "main.c", SRC_FOLDER "tf_alloc.c",
              SRC_FOLDER "tf_exec.c", SRC_FOLDER "tf_lexer.c",
              SRC_FOLDER "tf_lib.c", SRC_FOLDER "tf_obj.c");

    if (!compile_commands(&cmd, &targets, BUILD_FOLDER "compile_commands.json"))
        return 1;

    cc_output(&cmd, BUILD_FOLDER "toy_forth");
    cmd_extend(&cmd, &targets);
    if (!cmd_run(&cmd)) return 1;

    cmd_append(&cmd, BUILD_FOLDER "toy_forth");
    cmd_append(&cmd, "program.fth");
    if (!cmd_run(&cmd)) return 1;
    return 0;
}
