#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MINUTES_PER_DAY 1440
#define TOKEN_CAP 32

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INPUT_ERROR,
    STATUS_INVALID_FORMAT,
    STATUS_INVALID_VALUE,
    STATUS_OVERFLOW,
    STATUS_INVALID_TIME,
    STATUS_DUPLICATE_ID,
    STATUS_NO_MEMORY
} status_t;

typedef struct
{
    unsigned long id;
    int start;
    int finish;
} talk_t;

static status_t parse_positive(const char *str, unsigned long *out)
{
    if (str == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (str[0] == '\0')
    {
        return STATUS_INVALID_FORMAT;
    }

    unsigned long value = 0ul;
    for (size_t i = 0; str[i] != '\0'; i++)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            return STATUS_INVALID_FORMAT;
        }
        unsigned long digit = (unsigned long)(str[i] - '0');
        if (value > (ULONG_MAX - digit) / 10ul)
        {
            return STATUS_OVERFLOW;
        }
        value = value * 10ul + digit;
    }
    if (value == 0ul)
    {
        return STATUS_INVALID_VALUE;
    }
    *out = value;
    return STATUS_OK;
}

static status_t parse_time(const char *str, int *minutes)
{
    if (str == NULL || minutes == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    for (size_t i = 0; i < 5; i++)
    {
        if (i == 2)
        {
            if (str[i] != ':')
            {
                return STATUS_INVALID_TIME;
            }
        }
        else if (str[i] < '0' || str[i] > '9')
        {
            return STATUS_INVALID_TIME;
        }
    }
    if (str[5] != '\0')
    {
        return STATUS_INVALID_TIME;
    }

    int hours = (str[0] - '0') * 10 + (str[1] - '0');
    int mins = (str[3] - '0') * 10 + (str[4] - '0');
    if (hours > 23 || mins > 59)
    {
        return STATUS_INVALID_TIME;
    }
    *minutes = hours * 60 + mins;
    return STATUS_OK;
}

static int compare_by_id(const void *left, const void *right)
{
    const talk_t *a = (const talk_t *)left;
    const talk_t *b = (const talk_t *)right;
    if (a->id < b->id)
    {
        return -1;
    }
    return a->id > b->id;
}

static int compare_by_finish(const void *left, const void *right)
{
    const talk_t *a = (const talk_t *)left;
    const talk_t *b = (const talk_t *)right;
    if (a->finish != b->finish)
    {
        return a->finish < b->finish ? -1 : 1;
    }
    if (a->start != b->start)
    {
        return a->start < b->start ? -1 : 1;
    }
    return 0;
}

static status_t max_talks(const talk_t *talks, size_t count,
                          unsigned long halls, size_t *result)
{
    if (result == NULL || (talks == NULL && count > 0))
    {
        return STATUS_NULL_POINTER;
    }
    if (halls == 0ul)
    {
        return STATUS_INVALID_VALUE;
    }
    *result = 0;
    if (count == 0)
    {
        return STATUS_OK;
    }

    if (count > SIZE_MAX / sizeof(talk_t))
    {
        return STATUS_NO_MEMORY;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (talks[i].start < 0 || talks[i].finish >= MINUTES_PER_DAY ||
            talks[i].start >= talks[i].finish)
        {
            return STATUS_INVALID_TIME;
        }
    }

    talk_t *sorted = (talk_t *)malloc(count * sizeof(talk_t));
    if (sorted == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    for (size_t i = 0; i < count; i++)
    {
        sorted[i] = talks[i];
    }
    qsort(sorted, count, sizeof(talk_t), compare_by_finish);

    unsigned long halls_free_at[MINUTES_PER_DAY] = {0};
    halls_free_at[0] = halls;

    size_t chosen = 0;
    for (size_t i = 0; i < count; i++)
    {
        int moment = sorted[i].start;
        while (moment >= 0 && halls_free_at[moment] == 0ul)
        {
            moment--;
        }
        if (moment < 0)
        {
            continue;
        }
        halls_free_at[moment]--;
        halls_free_at[sorted[i].finish]++;
        chosen++;
    }

    free(sorted);
    *result = chosen;
    return STATUS_OK;
}

static status_t read_input(FILE *in, talk_t **talks, size_t *count,
                           unsigned long *halls)
{
    if (in == NULL || talks == NULL || count == NULL || halls == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *talks = NULL;
    *count = 0;
    *halls = 0ul;

    char first[TOKEN_CAP];
    char second[TOKEN_CAP];
    if (fscanf(in, "%31s %31s", first, second) != 2)
    {
        return STATUS_INPUT_ERROR;
    }

    unsigned long n = 0ul;
    unsigned long k = 0ul;
    status_t status = parse_positive(first, &n);
    if (status != STATUS_OK)
    {
        return status;
    }
    status = parse_positive(second, &k);
    if (status != STATUS_OK)
    {
        return status;
    }
    if (n > SIZE_MAX / sizeof(talk_t))
    {
        return STATUS_NO_MEMORY;
    }

    talk_t *array = (talk_t *)malloc((size_t)n * sizeof(talk_t));
    if (array == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    for (size_t i = 0; i < (size_t)n && status == STATUS_OK; i++)
    {
        char id_token[TOKEN_CAP];
        char start_token[TOKEN_CAP];
        char finish_token[TOKEN_CAP];
        if (fscanf(in, "%31s %31s %31s", id_token, start_token,
                   finish_token) != 3)
        {
            status = STATUS_INPUT_ERROR;
            break;
        }
        status = parse_positive(id_token, &array[i].id);
        if (status == STATUS_OK)
        {
            status = parse_time(start_token, &array[i].start);
        }
        if (status == STATUS_OK)
        {
            status = parse_time(finish_token, &array[i].finish);
        }
        if (status == STATUS_OK && array[i].start >= array[i].finish)
        {
            status = STATUS_INVALID_TIME;
        }
    }

    if (status == STATUS_OK)
    {
        qsort(array, (size_t)n, sizeof(talk_t), compare_by_id);
        for (size_t i = 1; i < (size_t)n; i++)
        {
            if (array[i - 1].id == array[i].id)
            {
                status = STATUS_DUPLICATE_ID;
                break;
            }
        }
    }

    if (status != STATUS_OK)
    {
        free(array);
        return status;
    }
    *talks = array;
    *count = (size_t)n;
    *halls = k;
    return STATUS_OK;
}

static const char *status_message(status_t status)
{
    switch (status)
    {
    case STATUS_OK:
        return "успех";
    case STATUS_NULL_POINTER:
        return "нулевой указатель";
    case STATUS_INPUT_ERROR:
        return "недостаточно данных во входном потоке";
    case STATUS_INVALID_FORMAT:
        return "ожидалось положительное целое число";
    case STATUS_INVALID_VALUE:
        return "значение должно быть положительным";
    case STATUS_OVERFLOW:
        return "число слишком велико";
    case STATUS_INVALID_TIME:
        return "некорректное время (нужен формат HH:MM, начало меньше конца)";
    case STATUS_DUPLICATE_ID:
        return "идентификаторы докладов должны быть уникальны";
    case STATUS_NO_MEMORY:
        return "не удалось выделить память";
    default:
        return "неизвестная ошибка";
    }
}

int main(void)
{
    talk_t *talks = NULL;
    size_t count = 0;
    unsigned long halls = 0ul;

    status_t status = read_input(stdin, &talks, &count, &halls);
    if (status != STATUS_OK)
    {
        fprintf(stderr, "Ошибка ввода: %s\n", status_message(status));
        return 1;
    }

    size_t result = 0;
    status = max_talks(talks, count, halls, &result);
    free(talks);
    if (status != STATUS_OK)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(status));
        return 1;
    }

    printf("%zu\n", result);
    return 0;
}