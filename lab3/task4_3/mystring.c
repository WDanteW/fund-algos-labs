#include "mystring.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static bool string_is_valid(const String *string)
{
    return string != NULL && string->data != NULL && string->length >= 0;
}

status_t string_create(String *result, const char *source)
{
    if (result == NULL || source == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    result->data = NULL;
    result->length = 0;

    size_t length = strlen(source);
    if (length >= (size_t)INT_MAX)
    {
        return STATUS_OVERFLOW;
    }
    char *buffer = (char *)malloc(length + 1);
    if (buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    memcpy(buffer, source, length + 1);
    result->data = buffer;
    result->length = (int)length;
    return STATUS_OK;
}

void string_destroy(String *string)
{
    if (string == NULL)
    {
        return;
    }
    free(string->data);
    string->data = NULL;
    string->length = 0;
}

status_t string_compare_lex(const String *left, const String *right,
                            int *result)
{
    if (result == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!string_is_valid(left) || !string_is_valid(right))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t common = (size_t)(left->length < right->length ? left->length
                                                           : right->length);
    int cmp = memcmp(left->data, right->data, common);
    if (cmp != 0)
    {
        *result = cmp < 0 ? -1 : 1;
    }
    else if (left->length != right->length)
    {
        *result = left->length < right->length ? -1 : 1;
    }
    else
    {
        *result = 0;
    }
    return STATUS_OK;
}

status_t string_compare_order(const String *left, const String *right,
                              int *result)
{
    if (result == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!string_is_valid(left) || !string_is_valid(right))
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (left->length != right->length)
    {
        *result = left->length < right->length ? -1 : 1;
        return STATUS_OK;
    }
    return string_compare_lex(left, right, result);
}

status_t string_equal(const String *left, const String *right, bool *result)
{
    if (result == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    int order = 0;
    status_t status = string_compare_lex(left, right, &order);
    if (status != STATUS_OK)
    {
        return status;
    }
    *result = (order == 0);
    return STATUS_OK;
}

status_t string_copy_into(String *destination, const String *source)
{
    if (destination == NULL || source == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!string_is_valid(source))
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (destination == source)
    {
        return STATUS_OK;
    }

    size_t length = (size_t)source->length;
    char *buffer = (char *)malloc(length + 1);
    if (buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    memcpy(buffer, source->data, length + 1);
    free(destination->data);
    destination->data = buffer;
    destination->length = source->length;
    return STATUS_OK;
}

status_t string_clone(String **result, const String *source)
{
    if (result == NULL || source == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *result = NULL;
    if (!string_is_valid(source))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    String *copy = (String *)malloc(sizeof(String));
    if (copy == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    copy->data = NULL;
    copy->length = 0;

    status_t status = string_copy_into(copy, source);
    if (status != STATUS_OK)
    {
        free(copy);
        return status;
    }
    *result = copy;
    return STATUS_OK;
}

void string_free(String **string)
{
    if (string == NULL || *string == NULL)
    {
        return;
    }
    string_destroy(*string);
    free(*string);
    *string = NULL;
}

status_t string_concat(String *destination, const String *source)
{
    if (!string_is_valid(destination) || !string_is_valid(source))
    {
        if (destination == NULL || source == NULL)
        {
            return STATUS_NULL_POINTER;
        }
        return STATUS_INVALID_ARGUMENT;
    }

    size_t destination_length = (size_t)destination->length;
    size_t source_length = (size_t)source->length;
    if (source_length > (size_t)INT_MAX - 1 - destination_length)
    {
        return STATUS_OVERFLOW;
    }
    size_t total = destination_length + source_length;

    char *buffer = (char *)realloc(destination->data, total + 1);
    if (buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    const char *source_data = (source == destination) ? buffer : source->data;
    memcpy(buffer + destination_length, source_data, source_length);
    buffer[total] = '\0';
    destination->data = buffer;
    destination->length = (int)total;
    return STATUS_OK;
}