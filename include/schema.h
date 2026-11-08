#ifndef SCHEMA_H
#define SCHEMA_H

#include <stdio.h>
#include "types.h"
#include "csv.h"

typedef struct {
    char **names;
    ColumnType *types;
    int count;
} Schema;

void schema_init(Schema *s);
void schema_free(Schema *s);

/* читает заголовок, инициализирует схему
   rows = 0 */
int schema_read_header(FILE *f, Schema *s, long *rows);

/* обновляет типы по записи
   возвращает 0 при успехе, 1 при ошибке */
int schema_update(Schema *s, CsvRecord *rec);

int schema_run(const char *filename);

#endif