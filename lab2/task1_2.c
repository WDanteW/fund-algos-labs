#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_FLAG,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION,
    ERROR_INVALID_SEED
} status_code;

static size_t my_strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

static int my_isdigit(char c) {
    return c >= '0' && c <= '9';
}

static int my_isalpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static char my_toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - ('a' - 'A'));
    }
    return c;
}

static status_code task_l(const char *str, size_t *len) {
    if (str == NULL || len == NULL) {
        return ERROR_INVALID_PARAM;
    }
    *len = my_strlen(str);
    return SUCCESS;
}

static status_code task_r(const char *str, char **result) {
    if (str == NULL || result == NULL) {
        return ERROR_INVALID_PARAM;
    }
    size_t len = my_strlen(str);
    char *res = (char *)malloc(len + 1);
    if (res == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    for (size_t i = 0; i < len; ++i) {
        res[i] = str[len - 1 - i];
    }
    res[len] = '\0';
    *result = res;
    return SUCCESS;
}

static status_code task_u(const char *str, char **result) {
    if (str == NULL || result == NULL) {
        return ERROR_INVALID_PARAM;
    }
    size_t len = my_strlen(str);
    char *res = (char *)malloc(len + 1);
    if (res == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    for (size_t i = 0; i < len; ++i) {
        if (i % 2 != 0) {
            res[i] = my_toupper(str[i]);
        } else {
            res[i] = str[i];
        }
    }
    res[len] = '\0';
    *result = res;
    return SUCCESS;
}

static status_code task_n(const char *str, char **result) {
    if (str == NULL || result == NULL) {
        return ERROR_INVALID_PARAM;
    }
    size_t len = my_strlen(str);
    char *res = (char *)malloc(len + 1);
    if (res == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    size_t idx = 0;
    for (size_t i = 0; i < len; ++i) {
        if (my_isdigit(str[i])) {
            res[idx++] = str[i];
        }
    }
    for (size_t i = 0; i < len; ++i) {
        if (my_isalpha(str[i])) {
            res[idx++] = str[i];
        }
    }
    for (size_t i = 0; i < len; ++i) {
        if (!my_isdigit(str[i]) && !my_isalpha(str[i])) {
            res[idx++] = str[i];
        }
    }
    res[len] = '\0';
    *result = res;
    return SUCCESS;
}

static status_code task_c(int argc, char *argv[], unsigned int seed, char **result) {
    if (argv == NULL || result == NULL) {
        return ERROR_INVALID_PARAM;
    }
    if (argc < 4) {
        return ERROR_INVALID_ARG_COUNT;
    }

    int num_strings = argc - 3;
    const char **strings = (const char **)malloc(num_strings * sizeof(char *));
    if (strings == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    size_t total_len = 0;
    for (int i = 0; i < num_strings; ++i) {
        strings[i] = argv[3 + i];
        total_len += my_strlen(strings[i]);
    }

    char *res = (char *)malloc(total_len + 1);
    if (res == NULL) {
        free(strings);
        return ERROR_MEMORY_ALLOCATION;
    }
    res[0] = '\0';

    srand(seed);

    int *indices = (int *)malloc(num_strings * sizeof(int));
    if (indices == NULL) {
        free(strings);
        free(res);
        return ERROR_MEMORY_ALLOCATION;
    }
    for (int i = 0; i < num_strings; ++i) {
        indices[i] = i;
    }

    for (int i = num_strings - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    size_t current_pos = 0;
    for (int i = 0; i < num_strings; ++i) {
        const char *s = strings[indices[i]];
        size_t s_len = my_strlen(s);
        for (size_t k = 0; k < s_len; ++k) {
            res[current_pos++] = s[k];
        }
    }
    res[total_len] = '\0';

    free(indices);
    free(strings);
    *result = res;
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Ошибка: недостаточно аргументов.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *flag = argv[1];
    status_code status = SUCCESS;

    if (flag[0] != '-' || flag[2] != '\0') {
        fprintf(stderr, "Ошибка: неверный формат флага.\n");
        return ERROR_INVALID_FLAG;
    }

    switch (flag[1]) {
        case 'l': {
            if (argc != 3) {
                fprintf(stderr, "Ошибка: для флага -l требуется ровно одна строка.\n");
                return ERROR_INVALID_ARG_COUNT;
            }
            size_t length = 0;
            status = task_l(argv[2], &length);
            if (status == SUCCESS) {
                printf("Длина строки: %zu\n", length);
            }
            break;
        }
        case 'r': {
            if (argc != 3) {
                fprintf(stderr, "Ошибка: для флага -r требуется ровно одна строка.\n");
                return ERROR_INVALID_ARG_COUNT;
            }
            char *reversed = NULL;
            status = task_r(argv[2], &reversed);
            if (status == SUCCESS) {
                printf("Перевернутая строка: %s\n", reversed);
                free(reversed);
            }
            break;
        }
        case 'u': {
            if (argc != 3) {
                fprintf(stderr, "Ошибка: для флага -u требуется ровно одна строка.\n");
                return ERROR_INVALID_ARG_COUNT;
            }
            char *modified = NULL;
            status = task_u(argv[2], &modified);
            if (status == SUCCESS) {
                printf("Результат (-u): %s\n", modified);
                free(modified);
            }
            break;
        }
        case 'n': {
            if (argc != 3) {
                fprintf(stderr, "Ошибка: для флага -n требуется ровно одна строка.\n");
                return ERROR_INVALID_ARG_COUNT;
            }
            char *rearranged = NULL;
            status = task_n(argv[2], &rearranged);
            if (status == SUCCESS) {
                printf("Результат (-n): %s\n", rearranged);
                free(rearranged);
            }
            break;
        }
        case 'c': {
            if (argc < 4) {
                fprintf(stderr, "Ошибка: для флага -c необходим seed и хотя бы одна строка.\n");
                return ERROR_INVALID_ARG_COUNT;
            }
            char *endptr;
            unsigned long seed_val = strtoul(argv[2], &endptr, 10);
            if (*endptr != '\0' || seed_val > UINT_MAX) {
                fprintf(stderr, "Ошибка: неверный формат seed.\n");
                return ERROR_INVALID_SEED;
            }

            char *concatenated = NULL;
            status = task_c(argc, argv, (unsigned int)seed_val, &concatenated);
            if (status == SUCCESS) {
                printf("Результат конкатенации (-c): %s\n", concatenated);
                free(concatenated);
            }
            break;
        }
        default:
            fprintf(stderr, "Ошибка: неизвестный флаг '-%c'.\n", flag[1]);
            status = ERROR_INVALID_FLAG;
            break;
    }

    return status;
}