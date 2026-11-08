#include "schema.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void schema_init(Schema *s) { 
    s->names = NULL; 
    s->types = NULL; 
    s->count = 0; 
}

void schema_free(Schema *s) {
    if (s->names) {
        for (int i = 0; i < s->count; i++) {
            free(s->names[i]);
        }
    }
    free(s->names);
    free(s->types);
    schema_init(s);
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

int schema_read_header(FILE *f, Schema *s, long *rows) {
    CsvRecord rec; csv_record_init(&rec);
    int r = csv_read_record(f, &rec);
    if (r <= 0) { 
        csv_record_free(&rec); 
        return 1; 
    }

    s->count = rec.count;
    s->names = calloc((size_t)rec.count, sizeof(char *));
    s->types = calloc((size_t)rec.count, sizeof(ColumnType));   
    if (!s->names || !s->types) { 
        csv_record_free(&rec); 
        return 1; 
    }

    for (int i = 0; i < rec.count; i++) {
        if (!rec.quoted[i] && csv_is_blank(rec.fields[i])) { 
            csv_record_free(&rec); 
            return 1; 
        }
        s->names[i] = xstrdup(rec.fields[i]);
        if (!s->names[i]) { 
            csv_record_free(&rec); 
            return 1; 
        }
        s->types[i] = TYPE_NULL;
    }

    for (int i = 0; i < rec.count; i++)
        for (int j = i + 1; j < rec.count; j++)
            if (strcmp(s->names[i], s->names[j]) == 0) { 
                csv_record_free(&rec); 
                return 1; 
            }

    csv_record_free(&rec);
    *rows = 0;
    return 0;
}

int schema_update(Schema *s, CsvRecord *rec) {
    if (rec->count != s->count) {
        return 1;
    }
    for (int i = 0; i < s->count; i++) {
        ColumnType ft = types_detect_field(rec->fields[i], rec->quoted[i]);
        s->types[i] = types_extend(s->types[i], ft);
    }
    return 0;
}

int schema_run(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { 
        fprintf(stderr, "Error: cannot open %s\n", filename); 
        return 1; 
    }

    Schema s; schema_init(&s);
    long rows = 0;
    if (schema_read_header(f, &s, &rows) != 0) {
        fclose(f);
        schema_free(&s);
        fprintf(stderr, "Error: invalid CSV header\n"); 
        return 1;
    }

    CsvRecord rec; csv_record_init(&rec);
    int ret;
    long pending_empty = 0;   // пустые строки в конце файла
    while ((ret = csv_read_record(f, &rec)) > 0) {
        if (rec.raw_length == 0 && rec.count == 1) {
            pending_empty++;
            csv_record_free(&rec);
            continue;
        }
        while (pending_empty > 0) {
            if (s.count != 1) { 
                fclose(f); 
                schema_free(&s); 
                csv_record_free(&rec); 
                return 1; 
            }
            rows++;
            pending_empty--;
        }
        if (schema_update(&s, &rec) != 0) {
            fclose(f); 
            schema_free(&s); 
            csv_record_free(&rec);
            fprintf(stderr, "Error: incorrect CSV format\n"); 
            return 1;
        }
        rows++;
        csv_record_free(&rec);
    }
    if (ret < 0) {
        fclose(f); 
        schema_free(&s);
        fprintf(stderr, "Error: incorrect CSV format\n"); 
        return 1;
    }

    printf("rows %ld\ncolumns %d\n", rows, s.count);
    for (int i = 0; i < s.count; i++) {
        printf("%d \"", i);
        for (const char *p = s.names[i]; *p; p++) {
            if (*p == '"') {
                putchar('"');
            }
            putchar(*p);
        }
        printf("\" %s\n", types_name(s.types[i]));
    }

    fclose(f); 
    schema_free(&s);
    return 0;
}