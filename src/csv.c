#include "csv.h"
#include <stdlib.h>
#include <string.h>


void csv_record_init(CsvRecord *rec) {
    rec->fields = NULL;
    rec->quoted = NULL;
    rec->count = 0;
    rec->capacity = 0;
    rec->raw_length = 0;
}


void csv_record_free(CsvRecord *rec) {
    if (rec->fields) {
        for (int i = 0; i < rec->count; i++) {
            free(rec->fields[i]);
        }
        free(rec->fields);
    }
    free(rec->quoted);
    csv_record_init(rec);
}


// «есть ли в rec место, чтобы добавить ещё одно поле? Если нет — расширь массивы fields и quoted так, чтобы место появилось»
/* возвращает: 
    0 - успех (место либо уже было, либо только что выделили)
    -1 - ошибка памяти (завершить работу с кодом 1)
*/
static int record_grow(CsvRecord *rec) {
    if (rec->count < rec->capacity) {
        return 0; // если места ещё достаточно
    }

    int new_capacity = rec->capacity ? rec->capacity * 2 : 1; // новая ёмкость

    char **new_fields = realloc(rec->fields, new_capacity * sizeof(char *)); // расширение массива полей
    if (!new_fields) {
        return -1; // не удалось выделить память :-(
    }
    rec->fields = new_fields; // удалось :-)

    int *new_quoted = realloc(rec->quoted, new_capacity * sizeof(int)); // расширение массива флагов (новое поле - новый флаг)
    if (!new_quoted) {
        return -1; // не удалось выделить память :-(
    }
    rec->quoted = new_quoted; // удалось :-)

    rec->capacity = new_capacity; // ёмкость удалось увеличить 
    return 0;
}


/* «возьми готовое значение поля и положи его в запись rec как новое поле, а флаг quoted — как признак, было ли оно в кавычках» 
   возвращает:
    0 - успех 
    -1 - ошибка памяти (завершить работу с кодом 1) 
*/
static int record_add_field(CsvRecord *rec, const char *value, int quoted) {
    if (record_grow(rec) == -1) return -1; // не удалось выделить память под новое поле

    char *copy = malloc(strlen(value) + 1); // сначала выделяем место под копию value на случай, если не удастся выделить память, чтобы не записывать NULL в rec->fields
    if (!copy) {
        return -1;
    }

    strcpy(copy, value);
    rec->fields[rec->count] = copy;
    rec->quoted[rec->count] = quoted;
    rec->count++;
    return 0;
}

 // обрезает хвостовые пробелы
static size_t remove_spaces_end(char *buf, size_t len) {
    while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t')) {
        len--;
    }
    return len;
}

int csv_read_record(FILE *f, CsvRecord *rec) {
    rec->count = 0; // сбрасываем счётчик полей
    rec->raw_length = 0;

    int c = fgetc(f); // первый символ
    if (c == EOF) {
        return 0; // файл ВСЁ (или был пуст)
    }

    size_t cap = 1; // начальная ёмкость буфера
    size_t len = 0; // длина текущего поля

    char *buf = malloc(cap * sizeof(char)); // массив для текущего поля
    if (!buf) {
        return -1;
    }

    int in_quotes = 0; // сейчас в кавычках?
    int field_started = 0; /* 1 - уже начали накапливать поле 
                              0 -  в фазе "пропуска ведущих пробелов" */
    int field_quoted = 0; // 1 - текущее поле было в кавычках

    for (;;) { // бесконечный цикл с явными break/continue
        if (c == EOF) {
            if (in_quotes) { 
                free(buf); 
                return -1; 
            }
            if (!field_quoted) { // если поле НЕ было в кавычках
                len = remove_spaces_end(buf, len);
            }
            buf[len] = '\0'; 
            if (record_add_field(rec, buf, field_quoted) != 0) {  // если не удалось записать новое поле в массив полей
                free(buf); 
                return -1; 
            }
            break;
        }

        if (in_quotes) { // если сейчас в кавычках
            if (c == '\n') { 
                free(buf);
                 return -1;
            }
            if (c == '"') { 
                int next = fgetc(f);
                if (next == '"') { // проверка: это кавычки внутри кавычек?
                    if (len + 1 >= cap) {
                        char *nb = realloc(buf, cap * 2);
                        if (!nb) { 
                            free(buf); 
                            return -1; 
                        }
                        buf = nb;
                        cap *= 2;
                    }
                    buf[len++] = '"';
                    rec->raw_length += 2;
                    c = fgetc(f);
                    continue;
                }
                in_quotes = 0; // это не кавычки внутри кавычек => это конец кавычек
                c = next;
                while (c == ' ' || c == '\t') { // проход по хвостовым пробелам и табуляциям
                    rec->raw_length++; 
                    c = fgetc(f); 
                }
                continue;
            }

            if (len + 1 >= cap) { 
                char *nb = realloc(buf, cap * 2);
                if (!nb) { 
                    free(buf); 
                    return -1; 
                }
                buf = nb;
                cap *= 2;
            }
            buf[len++] = (char)c;
            rec->raw_length++;
            c = fgetc(f);
            continue;
        }

        if (c == '\n') {
            if (!field_quoted) {
                len = remove_spaces_end(buf, len);
            }
            buf[len] = '\0';
            if (record_add_field(rec, buf, field_quoted) != 0) { 
                free(buf); 
                return -1; 
            }
            break;
        }

        if (c == ',') {
            if (!field_quoted) {
                len = remove_spaces_end(buf, len);
            }
            buf[len] = '\0';
            if (record_add_field(rec, buf, field_quoted) != 0) { 
                free(buf); 
                return -1; 
            }
            len = 0;
            field_started = 0;
            field_quoted = 0;
            rec->raw_length++;
            c = fgetc(f);
            continue;
        }

        if (c == '"') {
            if (!field_started) {
                field_quoted = 1;
                in_quotes = 1;
                field_started = 1;
                rec->raw_length++;
                c = fgetc(f);
                continue;
            }
            free(buf);
            return -1;   // кавычка внутри обычного поля — плохо
        }

        if (c == ' ' || c == '\t') {
            rec->raw_length++;
            if (!field_started) { 
                c = fgetc(f); 
                continue; 
            }
            if (len + 1 >= cap) { 
                char *nb = realloc(buf, cap * 2);
                if (!nb) { 
                    free(buf); 
                    return -1; 
                }
                buf = nb;
                cap *= 2;
            }
            buf[len++] = (char)c;
            c = fgetc(f);
            continue;
        }

        field_started = 1;
        rec->raw_length++;
        if (len + 1 >= cap) { 
            char *nb = realloc(buf, cap * 2);
            if (!nb) { 
                free(buf); 
                return -1; 
            }
            buf = nb;
            cap *= 2;
        }
        buf[len++] = (char)c;
        c = fgetc(f);
    }

    free(buf);
    return 1;
}

int csv_is_blank(const char *s) {
    if (s == NULL) {
        return 1; // если указатель нулевой — строка пуста
    }
    for (; *s != '\0'; s++) {
        if (*s != ' ' && *s != '\t') return 0;
    }
    return 1;
}