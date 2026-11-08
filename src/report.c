#include "report.h"
#include "schema.h"
#include "stats.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int report_run(const char *in, const char *outname) {
    // проход 1
    FILE *fi = fopen(in, "r");
    if (!fi) {
        fprintf(stderr, "Error: cannot open %s\n", in);
        return 1;
    }

    Schema s; 
    schema_init(&s);
    long dummy = 0;
    if (schema_read_header(fi, &s, &dummy) != 0) {
        fclose(fi);
        schema_free(&s);
        fprintf(stderr, "Error: invalid CSV header\n");
        return 1;
    }

    CsvRecord rec; 
    csv_record_init(&rec);
    int ret;
    long pending = 0;
    while ((ret = csv_read_record(fi, &rec)) > 0) {
        if (rec.raw_length == 0 && rec.count == 1) {
            pending++;
            csv_record_free(&rec);
            continue;
        }
        while (pending > 0) {
            if (s.count != 1) {
                fclose(fi);
                schema_free(&s);
                csv_record_free(&rec);
                fprintf(stderr, "Error: incorrect CSV format\n");
                return 1;
            }
            pending--;
        }
        if (schema_update(&s, &rec) != 0) {
            fclose(fi);
            schema_free(&s);
            csv_record_free(&rec);
            fprintf(stderr, "Error: incorrect CSV format\n");
            return 1;
        }
        csv_record_free(&rec);
    }
    if (ret < 0) {
        fclose(fi);
        schema_free(&s);
        fprintf(stderr, "Error: incorrect CSV format\n");
        return 1;
    }
    fclose(fi);

    // проход 2
    fi = fopen(in, "r");
    if (!fi) {
        schema_free(&s);
        return 1;
    }

    CsvRecord hrec; 
    csv_record_init(&hrec);
    if (csv_read_record(fi, &hrec) != 1) {
        fclose(fi);
        csv_record_free(&hrec);
        schema_free(&s);
        return 1;
    }
    csv_record_free(&hrec);

    ColumnStats *st = malloc((size_t)s.count * sizeof(ColumnStats));
    if (!st) {
        fclose(fi);
        schema_free(&s);
        return 1;
    }
    for (int i = 0; i < s.count; i++) {
        stats_init(&st[i], s.types[i]);
    }

    long rows = 0;
    pending = 0;
    while ((ret = csv_read_record(fi, &rec)) > 0) {
        if (rec.raw_length == 0 && rec.count == 1) {
            pending++;
            csv_record_free(&rec);
            continue;
        }
        while (pending > 0) {
            if (s.count != 1) {
                fclose(fi);
                free(st);
                schema_free(&s);
                csv_record_free(&rec);
                fprintf(stderr, "Error: incorrect CSV format\n");
                return 1;
            }
            for (int i = 0; i < s.count; i++) {
                stats_update(&st[i], s.types[i], "", 0);
            }
            rows++;
            pending--;
        }
        if (rec.count != s.count) {
            fclose(fi);
            free(st);
            schema_free(&s);
            csv_record_free(&rec);
            fprintf(stderr, "Error: incorrect CSV format\n");
            return 1;
        }
        for (int i = 0; i < s.count; i++) {
            stats_update(&st[i], s.types[i], rec.fields[i], rec.quoted[i]);
        }
        rows++;
        csv_record_free(&rec);
    }
    if (ret < 0) {
        fclose(fi);
        free(st);
        schema_free(&s);
        fprintf(stderr, "Error: incorrect CSV format\n");
        return 1;
    }
    fclose(fi);

    // запись отчёта
    FILE *fo = fopen(outname, "w");
    if (!fo) {
        free(st);
        schema_free(&s);
        fprintf(stderr, "Error: cannot open %s\n", outname);
        return 1;
    }

    fprintf(fo, "CSV Report\n\n");
    fprintf(fo, "File: %s\nRows: %ld\nColumns: %d\n\n", in, rows, s.count);

    for (int i = 0; i < s.count; i++) {
        fprintf(fo, "Column %d: %s (%s)\n", i, s.names[i], types_name(s.types[i]));
        ColumnStats *c = &st[i];
        if (s.types[i] == TYPE_NULL) {
            fprintf(fo, "Rows: %lld\n", c->null_rows);
        }
        else {
            fprintf(fo, "Nulls: %lld\n", c->nulls);
            long long n = c->non_null;
            if (s.types[i] == TYPE_INT) {
                fprintf(fo, "Min: %lld\n", n ? c->int_min : 0LL);
                fprintf(fo, "Max: %lld\n", n ? c->int_max : 0LL);
                fprintf(fo, "Mean: %.6f\n", (double)(n ? c->int_sum / (long double)n : 0.0L));
                fprintf(fo, "Positive: %lld\nNegative: %lld\nEven: %lld\n", c->int_positive, c->int_negative, c->int_even);
            }
            else if (s.types[i] == TYPE_DOUBLE) {
                fprintf(fo, "Min: %.6f\n", n ? c->double_min : 0.0);
                fprintf(fo, "Max: %.6f\n", n ? c->double_max : 0.0);
                fprintf(fo, "Mean: %.6f\n", (double)(n ? c->double_sum / (long double)n : 0.0L));
                fprintf(fo, "Positive: %lld\nNegative: %lld\n", c->double_positive, c->double_negative);
            }
            else {
                fprintf(fo, "Min len: %zu\n", n ? c->str_min_len : 0);
                fprintf(fo, "Max len: %zu\n", n ? c->str_max_len : 0);
                fprintf(fo, "Mean len: %.6f\n", (double)(n ? c->str_sum_len / (long double)n : 0.0L));
                fprintf(fo, "Empty: %lld\n", c->str_empty);
            }
        }
        fputc('\n', fo);
    }

    fclose(fo);
    free(st);
    schema_free(&s);
    return 0;
}