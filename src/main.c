#include <stdio.h>
#include "cli.h"
#include "schema.h"
#include "stats.h"
#include "report.h"

static void print_help(void);

int main(int argc, char *argv[]) {
    CliArgs args;

    int cli_result = cli_parse(argc, argv, &args);

    if (cli_result != 0) {
        fprintf(stderr, "Error: unknown command or incorrect number of arguments\n");
        return 2;
    }

    switch (args.cmd) {
        case cmd_help:
            print_help();
            return 0;
        case cmd_schema:
            return schema_run(args.input_file);
        case cmd_stats:
            return stats_run(args.input_file);
        case cmd_report:
            return report_run(args.input_file, args.output_file);
        default:
            return 2;
    }
}

static void print_help(void) {
    printf("Usage: csvstat --help\n");
    printf("       csvstat schema <csv-file>\n");
    printf("       csvstat stats <csv-file>\n");
    printf("       csvstat report <csv-file> <output-file>\n");
}