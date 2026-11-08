#include "cli.h"
#include <string.h>
#include <stdio.h>

int cli_parse(int argc, char *argv[], CliArgs *args) {
    args->cmd = cmd_unknown;
    args->input_file = NULL; // на всякий случай, если main ВДРУГ захочет использовать его, а там будет мусор
    args->output_file = NULL;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        args->cmd = cmd_help;
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "schema") == 0) {
        args->cmd = cmd_schema;
        args->input_file = argv[2];
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "stats") == 0) {
        args->cmd = cmd_stats;
        args->input_file = argv[2];
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "report") == 0) {
        args->cmd = cmd_report;
        args->input_file = argv[2];
        args->output_file = argv[3];
        return 0;
    }

    // неизвестная команда или неверное число аргументов
    return 2;
}