#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include "types.h"

typedef struct {
    long long nulls;
    long long non_null;

    // INT
    long long int_min, int_max;
    long double int_sum;
    long long int_positive, int_negative, int_even;

    // DOUBLE
    double double_min, double_max;
    long double double_sum;
    long long double_positive, double_negative;

    // STRING
    size_t str_min_len, str_max_len;
    long double str_sum_len;
    long long str_empty;

    // NULL
    long long null_rows;
} ColumnStats;

void stats_init(ColumnStats *st, ColumnType type);
void stats_update(ColumnStats *st, ColumnType col_type, const char *field, int quoted);
void stats_print(const ColumnStats *st, ColumnType type, int index, const char *name);

int stats_run(const char *filename);

#endif