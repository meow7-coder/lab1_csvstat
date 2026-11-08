#include "stats.h"
#include "csv.h"
#include "schema.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>

void stats_init(ColumnStats *st, ColumnType type) {
    memset(st, 0, sizeof(*st));
    if (type == TYPE_INT) { 
        st->int_min = LLONG_MAX; 
        st->int_max = LLONG_MIN; 
    }
    if (type == TYPE_DOUBLE) { 
        st->double_min = INFINITY; 
        st->double_max = -INFINITY; 
    }
    if (type == TYPE_STRING) { 
        st->str_min_len = SIZE_MAX; 
    }
}

void stats_update(ColumnStats *st, ColumnType col_type, const char *field, int quoted) {
    if (col_type == TYPE_NULL) { 
        st->null_rows++; 
        return; 
    }

    ColumnType ft = types_detect_field(field, quoted);
    if (ft == TYPE_NULL) { 
        st->nulls++; 
        return; 
    }
    st->non_null++;

    if (col_type == TYPE_INT && ft == TYPE_INT) {
        char *end = NULL;
        long long v = strtoll(field, &end, 10);

        if (!end || *end != '\0')
            return;
        if (v < st->int_min)
            st->int_min = v;
        if (v > st->int_max)
            st->int_max = v;
        st->int_sum += (long double)v;
        if (v > 0)
            st->int_positive++;
        if (v < 0)
            st->int_negative++;
        if (v % 2 == 0)
            st->int_even++;
    } 
    else if (col_type == TYPE_DOUBLE && ft == TYPE_DOUBLE) {
        char *end = NULL;
        double v = strtod(field, &end);

        if (!end || *end != '\0') 
            return;
        if (v < st->double_min) 
            st->double_min = v;
        if (v > st->double_max) 
            st->double_max = v;
        st->double_sum += (long double)v;
        if (v > 0.0) 
            st->double_positive++;
        if (v < 0.0)   
            st->double_negative++;
    } 
    else if (col_type == TYPE_STRING) {
        size_t len = strlen(field);

        if (len < st->str_min_len) 
            st->str_min_len = len;
        if (len > st->str_max_len) 
            st->str_max_len = len;
        st->str_sum_len += (long double)len;
        if (len == 0) 
            st->str_empty++;
    }
}

static void print_name_escaped(const char *name) {
    putchar('"');
    for (const char *p = name; *p; p++) {
        if (*p == '"') {
            putchar('"');
        }
        putchar(*p);
    }
    putchar('"');
}

void stats_print(const ColumnStats *st, ColumnType type, int index, const char *name) {
    printf("column %d\nname ", index);
    print_name_escaped(name);
    printf("\ntype %s\n", types_name(type));

    if (type == TYPE_NULL) { 
        printf("rows %lld\n", st->null_rows); 
        return; 
    }

    printf("nulls %lld\n", st->nulls);
    long long n = st->non_null;
    if (type == TYPE_INT) {
        printf("min %lld\n", n ? st->int_min : 0LL);
        printf("max %lld\n", n ? st->int_max : 0LL);
        long double m = n ? st->int_sum / (long double)n : 0.0L;
        printf("mean %.6f\n", (double)m);
        printf("positive %lld\nnegative %lld\neven %lld\n", st->int_positive, st->int_negative, st->int_even);
    } 
    else if (type == TYPE_DOUBLE) {
        printf("min %.6f\n", n ? st->double_min : 0.0);
        printf("max %.6f\n", n ? st->double_max : 0.0);
        long double m = n ? st->double_sum / (long double)n : 0.0L;
        printf("mean %.6f\n", (double)m);
        printf("positive %lld\nnegative %lld\n", st->double_positive, st->double_negative);
    } 
    else {
        printf("min_length %zu\n", n ? st->str_min_len : 0);
        printf("max_length %zu\n", n ? st->str_max_len : 0);
        long double m = n ? st->str_sum_len / (long double)n : 0.0L;
        printf("mean_length %.6f\n", (double)m);
        printf("empty %lld\n", st->str_empty);
    }
}

int stats_run(const char *filename) {
    // проход 1
    FILE *f = fopen(filename, "r");
    if (!f) { 
        fprintf(stderr, "Error: cannot open %s\n", filename); 
        return 1; 
    }

    Schema schema;
    schema_init(&schema);
    long dummy = 0;
    if (schema_read_header(f, &schema, &dummy) != 0) {
        fclose(f); 
        schema_free(&schema);
        fprintf(stderr, "Error: invalid CSV header\n"); 
        return 1;
    }

    CsvRecord rec;
    csv_record_init(&rec);
    int ret;
    long pending = 0;
    while ((ret = csv_read_record(f, &rec)) > 0) {
        if (rec.raw_length == 0 && rec.count == 1) { 
            pending++; 
            csv_record_free(&rec); 
            continue; 
        }
        if (pending > 0 && schema.count != 1) {
            fclose(f); 
            schema_free(&schema); 
            csv_record_free(&rec);
            fprintf(stderr, "Error: incorrect CSV format\n"); 
            return 1;
        }
        pending = 0;
        if (schema_update(&schema, &rec) != 0) {
            fclose(f); 
            schema_free(&schema); 
            csv_record_free(&rec);
            fprintf(stderr, "Error: incorrect CSV format\n"); 
            return 1;
        }
        csv_record_free(&rec);
    }
    if (ret < 0) {
        fclose(f); 
        schema_free(&schema);
        fprintf(stderr, "Error: incorrect CSV format\n"); 
        return 1;
    }
    fclose(f);

    // проход 2
    f = fopen(filename, "r");
    if (!f) { 
        schema_free(&schema); 
        return 1; 
    }

    CsvRecord hrec; 
    csv_record_init(&hrec);
    if (csv_read_record(f, &hrec) <= 0) {
        fclose(f); 
        csv_record_free(&hrec); 
        schema_free(&schema);
        return 1;
    }
    csv_record_free(&hrec);

    ColumnStats *st = malloc((size_t)schema.count * sizeof(ColumnStats));
    if (!st) { 
        fclose(f); 
        schema_free(&schema); 
        return 1; 
    }
    for (int i = 0; i < schema.count; i++) 
        stats_init(&st[i], schema.types[i]);

    long rows = 0;
    pending = 0;
    while ((ret = csv_read_record(f, &rec)) > 0) {
        if (rec.raw_length == 0 && rec.count == 1) { 
            pending++; 
            csv_record_free(&rec); 
            continue; 
        }
        while (pending > 0) {
            if (schema.count != 1) { 
                fclose(f); 
                free(st); 
                schema_free(&schema); 
                csv_record_free(&rec);
                fprintf(stderr, "Error: incorrect CSV format\n"); 
                return 1;
            }
            for (int i = 0; i < schema.count; i++) 
                stats_update(&st[i], schema.types[i], "", 0);
            rows++; 
            pending--;
        }
        if (rec.count != schema.count) { 
            fclose(f); 
            free(st); 
            schema_free(&schema); 
            csv_record_free(&rec);
            fprintf(stderr, "Error: incorrect CSV format\n"); 
            return 1;
        }
        for (int i = 0; i < schema.count; i++) 
            stats_update(&st[i], schema.types[i], rec.fields[i], rec.quoted[i]);
        rows++; 
        csv_record_free(&rec);
    }
    if (ret < 0) {
        fclose(f); 
        free(st); 
        schema_free(&schema);
        fprintf(stderr, "Error: incorrect CSV format\n"); 
        return 1;
    }
    fclose(f);

    // вывод
    printf("rows %ld\ncolumns %d\n\n", rows, schema.count);
    for (int i = 0; i < schema.count; i++) {
        if (i > 0) putchar('\n');
        stats_print(&st[i], schema.types[i], i, schema.names[i]);
    }

    free(st); 
    schema_free(&schema);
    return 0;
}