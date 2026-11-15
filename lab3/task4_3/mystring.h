#ifndef MYSTRING_H
#define MYSTRING_H

#include <stdbool.h>

#include "status.h"

typedef struct
{
    char *data;
    int length;
} String;

status_t string_create(String *result, const char *source);
void string_destroy(String *string);
status_t string_compare_order(const String *left, const String *right,
                              int *result);
status_t string_compare_lex(const String *left, const String *right,
                            int *result);
status_t string_equal(const String *left, const String *right, bool *result);
status_t string_copy_into(String *destination, const String *source);
status_t string_clone(String **result, const String *source);
void string_free(String **string);
status_t string_concat(String *destination, const String *source);

#endif