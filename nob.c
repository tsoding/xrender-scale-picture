#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    bool run   = !false;
    bool debug = false;

    Cmd cmd = {0};
    cmd_append(&cmd, "clang");
    cmd_append(&cmd, "-Wall");
    cmd_append(&cmd, "-Wextra");
    if (debug) cmd_append(&cmd, "-ggdb");
    cmd_append(&cmd, "-o", "main");
    cmd_append(&cmd, "main.c");
    cmd_append(&cmd, "-lX11");
    cmd_append(&cmd, "-lXrender");
    cmd_append(&cmd, "-lm");
    if (!cmd_run(&cmd)) return 1;

    if (run) {
        if (debug) cmd_append(&cmd, "gf2");
        cmd_append(&cmd, "./main");
        cmd_append(&cmd, "./coding.png");
        if (!cmd_run(&cmd)) return 1;
    }

    return 0;
}
