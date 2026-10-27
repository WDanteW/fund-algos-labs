#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_FLAG,
    ERROR_INVALID_NUMBER,
    ERROR_OUT_OF_RANGE,
    ERROR_NO_MULTIPLES,
    ERROR_OVERFLOW
} status_code;

typedef enum prime_status {
    PRIME,
    COMPOSITE,
    NEITHER_PRIME_NOR_COMPOSITE
} prime_status;


static status_code parse_long(const char *str, long *result) {
    if (str == NULL || result == NULL || *str == '\0') {
        return ERROR_INVALID_NUMBER;
    }

    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);

    if (endptr == str) {
        return ERROR_INVALID_NUMBER;
    }

    if (*endptr != '\0') {
        return ERROR_INVALID_NUMBER;
    }

    if (val > INT_MAX || val < INT_MIN) {
        return ERROR_OUT_OF_RANGE;
    }

    *result = val;
    return SUCCESS;
}


static status_code task_h(long x, long *multiples, size_t *count) {
    if (x <= 0) {
        return ERROR_INVALID_NUMBER;
    }
    if (multiples == NULL || count == NULL) {
        return ERROR_INVALID_NUMBER;
    }

    *count = 0;
    for (long i = x; i <= 100; i += x) {
        multiples[(*count)++] = i;
    }

    if (*count == 0) {
        return ERROR_NO_MULTIPLES;
    }

    return SUCCESS;
}


static status_code task_p(long x, prime_status *res) {
    if (res == NULL) {
        return ERROR_INVALID_NUMBER;
    }

    if (x <= 1) {
        *res = NEITHER_PRIME_NOR_COMPOSITE;
        return SUCCESS;
    }

    for (long i = 2; i * i <= x; ++i) {
        if (x % i == 0) {
            *res = COMPOSITE;
            return SUCCESS;
        }
    }

    *res = PRIME;
    return SUCCESS;
}


static status_code task_s(long x, char *hex_str, size_t max_len) {
    if (hex_str == NULL || max_len == 0) {
        return ERROR_INVALID_NUMBER;
    }

    unsigned long abs_x = (x < 0) ? (unsigned long)(-x) : (unsigned long)x;

    snprintf(hex_str, max_len, "%lX", abs_x);
    return SUCCESS;
}


static status_code task_e(long x, long long table[10][10]) {
    if (x < 1 || x > 10) {
        return ERROR_OUT_OF_RANGE;
    }

    for (int base = 1; base <= 10; ++base) {
        long long current_power = 1;
        for (int exp = 1; exp <= x; ++exp) {
            current_power *= base;
            table[base - 1][exp - 1] = current_power;
        }
    }

    return SUCCESS;
}


static status_code task_a(long x, long long *sum) {
    if (x < 1) {
        return ERROR_INVALID_NUMBER;
    }
    if (sum == NULL) {
        return ERROR_INVALID_NUMBER;
    }

    if (x > 3037000499L) {
        return ERROR_OVERFLOW;
    }

    *sum = (long long)x * (x + 1) / 2;
    return SUCCESS;
}


static status_code task_f(long x, unsigned long long *factorial) {
    if (x < 0) {
        return ERROR_INVALID_NUMBER;
    }
    if (x > 20) {
        return ERROR_OVERFLOW;
    }
    if (factorial == NULL) {
        return ERROR_INVALID_NUMBER;
    }

    unsigned long long res = 1;
    for (long i = 1; i <= x; ++i) {
        res *= i;
    }

    *factorial = res;
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Ошибка: неверное количество аргументов.\nИспользование: %s <число_x> <флаг>\n", argv[0]);
        return ERROR_INVALID_ARG_COUNT;
    }

    long x = 0;
    status_code status = parse_long(argv[1], &x);
    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка: некорректный числовой аргумент x.\n");
        return status;
    }

    const char *flag_str = argv[2];
    if ((flag_str[0] != '-' && flag_str[0] != '/') || flag_str[1] == '\0' || flag_str[2] != '\0') {
        fprintf(stderr, "Ошибка: неверный формат флага. Флаг должен начинаться с '-' или '/' и состоять из одного символа.\n");
        return ERROR_INVALID_FLAG;
    }

    char flag = flag_str[1];

    switch (flag) {
        case 'h': {
            long multiples[100];
            size_t count = 0;
            status = task_h(x, multiples, &count);
            if (status == ERROR_INVALID_NUMBER) {
                fprintf(stderr, "Ошибка: для флага -h число x должно быть положительным натуральным числом.\n");
            } else if (status == ERROR_NO_MULTIPLES) {
                printf("В пределах 100 нет чисел, кратных %ld.\n", x);
            } else if (status == SUCCESS) {
                printf("Числа в пределах 100, кратные %ld: ", x);
                for (size_t i = 0; i < count; ++i) {
                    printf("%ld ", multiples[i]);
                }
                printf("\n");
            }
            break;
        }

        case 'p': {
            prime_status res;
            status = task_p(x, &res);
            if (status == SUCCESS) {
                if (res == PRIME) {
                    printf("Число %ld является ПРОСТЫМ.\n", x);
                } else if (res == COMPOSITE) {
                    printf("Число %ld является СОСТАВНЫМ.\n", x);
                } else {
                    printf("Число %ld не является ни простым, ни составным.\n", x);
                }
            }
            break;
        }

        case 's': {
            char hex_str[64];
            status = task_s(x, hex_str, sizeof(hex_str));
            if (status == SUCCESS) {
                printf("Цифра(-ы) числа %ld в 16-ричной системе счисления: ", x);
                for (size_t i = 0; hex_str[i] != '\0'; ++i) {
                    printf("%c ", hex_str[i]);
                }
                printf("\n");
            }
            break;
        }

        case 'e': {
            long long table[10][10];
            status = task_e(x, table);
            if (status == ERROR_OUT_OF_RANGE) {
                fprintf(stderr, "Ошибка: для флага -e число x должно быть в диапазоне от 1 до 10 включительно.\n");
            } else if (status == SUCCESS) {
                printf("Таблица степеней (основания 1..10, показатели 1..%ld):\n", x);
                printf("%-10s", "Основание");
                for (int exp = 1; exp <= x; ++exp) {
                    printf("^%-7d", exp);
                }
                printf("\n--------------------------------------------------------------------------\n");

                for (int base = 1; base <= 10; ++base) {
                    printf("%-10d", base);
                    for (int exp = 1; exp <= x; ++exp) {
                        printf("%-8lld", table[base - 1][exp - 1]);
                    }
                    printf("\n");
                }
            }
            break;
        }

        case 'a': {
            long long sum = 0;
            status = task_a(x, &sum);
            if (status == ERROR_INVALID_NUMBER) {
                fprintf(stderr, "Ошибка: для флага -a число x должно быть натуральным (>= 1).\n");
            } else if (status == ERROR_OVERFLOW) {
                fprintf(stderr, "Ошибка: вычисление суммы приводит к переполнению типа данных.\n");
            } else if (status == SUCCESS) {
                printf("Сумма натуральных чисел от 1 до %ld равна: %lld\n", x, sum);
            }
            break;
        }

        case 'f': {
            unsigned long long factorial = 0;
            status = task_f(x, &factorial);
            if (status == ERROR_INVALID_NUMBER) {
                fprintf(stderr, "Ошибка: для флага -f число x должно быть неотрицательным (>= 0).\n");
            } else if (status == ERROR_OVERFLOW) {
                fprintf(stderr, "Ошибка: переполнение при вычислении факториала (x > 20).\n");
            } else if (status == SUCCESS) {
                printf("Факториал числа %ld (%ld!) равен: %llu\n", x, x, factorial);
            }
            break;
        }

        default:
            fprintf(stderr, "Ошибка: неизвестный флаг '%c'.\n", flag);
            status = ERROR_INVALID_FLAG;
            break;
    }

    return status;
}