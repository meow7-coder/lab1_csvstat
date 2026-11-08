#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

static int is_int(const char *s) {
    if (!s || !*s) 
        return 0;
    const char *p = s;
    if (*p == '+' || *p == '-') 
        p++;
    if (*p < '0' || *p > '9') 
        return 0;
    while (*p >= '0' && *p <= '9') {
        p++;
    }
    if (*p != '\0') 
        return 0;
    errno = 0;
    char *end = NULL;
    strtoll(s, &end, 10);
    if (errno == ERANGE) 
        return 0;
    return end && *end == '\0';
}

static int is_double(const char *s) {
    if (!s || !*s) 
        return 0;
    const char *p = s;
    if (*p == '+' || *p == '-') 
        p++;
    int db = 0; while (*p >= '0' && *p <= '9') { 
        p++; 
        db++; 
    }
    if (db == 0 || *p != '.') 
        return 0;
    p++;
    int da = 0; 
    while (*p >= '0' && *p <= '9') { 
        p++; 
        da++; 
    }
    if (da == 0 || *p != '\0') 
        return 0;

    char *end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (errno == ERANGE || *end != '\0') 
        return 0;
    if (isnan(v) || isinf(v)) 
        return 0;
    return 1;
}

ColumnType types_detect_field(const char *field, int quoted) {
    if (quoted) {
        if (is_int(field)) 
            return TYPE_INT;
        if (is_double(field)) 
            return TYPE_DOUBLE;
        return TYPE_STRING;
    }

    const char *s = field;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    const char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t')) {
        e--;
    }
    size_t n = (size_t)(e - s);
    if (n == 0) 
        return TYPE_NULL;

    char *tmp = malloc(n + 1);
    if (!tmp) 
        return TYPE_STRING;
    memcpy(tmp, s, n);
    tmp[n] = '\0';

    ColumnType t;
    if (is_int(tmp)) 
        t = TYPE_INT;
    else if (is_double(tmp))
        t = TYPE_DOUBLE;
    else 
        t = TYPE_STRING;

    free(tmp);
    return t;
}

const char *types_name(ColumnType t) {
    switch (t) {
    case TYPE_NULL: 
        return "NULL";
    case TYPE_INT: 
        return "INT";
    case TYPE_DOUBLE: 
        return "DOUBLE";
    case TYPE_STRING: 
        return "STRING";
    default:
        return "UNKNOWN";
    }
}

ColumnType types_extend(ColumnType cur, ColumnType fld) {
    if (cur == TYPE_STRING || fld == TYPE_STRING) 
        return TYPE_STRING;
    if (cur == TYPE_DOUBLE || fld == TYPE_DOUBLE) 
        return TYPE_DOUBLE;
    if (cur == TYPE_INT || fld == TYPE_INT) 
        return TYPE_INT;
    return TYPE_NULL;
}