#ifndef CLI_H
#define CLI_H

typedef enum {
    cmd_help,
    cmd_schema,
    cmd_stats,
    cmd_report,
    cmd_unknown
} Command; // перечисление режимов работы

typedef struct {
    Command cmd;
    const char *input_file;
    const char *output_file; // только для report
} CliArgs; // структура для хранения аргументов коммандной строки

/* разбирает аргументы командной строки.
   возвращает: 
    0 - успех
    2 - ошибка аргументов
*/
int cli_parse(int argc, char *argv[], CliArgs *args);

#endif