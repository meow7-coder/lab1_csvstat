#ifndef TYPES_H
#define TYPES_H

typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING
} ColumnType;

// определяет тип поля по строковому представлению
// quoted — было ли поле заключено в двойные кавычки
ColumnType types_detect_field(const char *field, int quoted);

// возвращает строковое имя типа
const char *types_name(ColumnType t);

// расширяет тип столбца новым типом поля
ColumnType types_extend(ColumnType current, ColumnType field);

#endif