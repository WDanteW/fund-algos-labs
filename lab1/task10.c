#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_INPUT,
    ERROR_MEMORY_ALLOCATION,
    ERROR_OVERFLOW
} status_code;

static status_code parse_int(const char *str, int *result) {
    if (str == NULL || result == NULL || *str == '\0') return ERROR_INVALID_INPUT;
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);
    if (endptr == str || *endptr != '\0' || val > INT_MAX || val < INT_MIN) return ERROR_INVALID_INPUT;
    *result = (int)val;
    return SUCCESS;
}

static status_code read_line(char **line_out) {
    size_t cap = 64;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (buf == NULL) return ERROR_MEMORY_ALLOCATION;

    int ch = 0;
    while ((ch = getchar()) != EOF && ch != '\n' && ch != '\r') {
        if (len + 1 >= cap) {
            cap *= 2;
            char *new_buf = (char *)realloc(buf, cap);
            if (new_buf == NULL) {
                free(buf);
                return ERROR_MEMORY_ALLOCATION;
            }
            buf = new_buf;
        }
        buf[len++] = (char)ch;
    }

    if (ch == '\r') {
        int next = getchar();
        if (next != '\n' && next != EOF) ungetc(next, stdin);
    }

    if (ch == EOF && len == 0) {
        free(buf);
        *line_out = NULL;
        return SUCCESS;
    }

    buf[len] = '\0';
    *line_out = buf;
    return SUCCESS;
}

static char *clone_string(const char *str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str);
    char *res = (char *)malloc(len + 1);
    if (res == NULL) return NULL;
    memcpy(res, str, len + 1);
    return res;
}

static status_code parse_custom_number(const char *str, int base, long long *val_out) {
    if (str == NULL || val_out == NULL) return ERROR_INVALID_INPUT;

    size_t start = 0;
    bool negative = false;

    if (str[0] == '-') { negative = true; start = 1; }
    else if (str[0] == '+') { start = 1; }

    if (str[start] == '\0') return ERROR_INVALID_INPUT;

    unsigned long long result = 0;
    for (size_t i = start; str[i] != '\0'; i++) {
        char c = str[i];
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else return ERROR_INVALID_INPUT;

        if (digit >= base) return ERROR_INVALID_INPUT;

        if (__builtin_mul_overflow(result, (unsigned long long)base, &result) ||
            __builtin_add_overflow(result, (unsigned long long)digit, &result)) {
            return ERROR_OVERFLOW;
        }
    }

    if (negative) {
        if (result > (unsigned long long)LLONG_MAX + 1ULL) return ERROR_OVERFLOW;
        *val_out = (result == (unsigned long long)LLONG_MAX + 1ULL) ? LLONG_MIN : -(long long)result;
    } else {
        if (result > (unsigned long long)LLONG_MAX) return ERROR_OVERFLOW;
        *val_out = (long long)result;
    }

    return SUCCESS;
}

static void to_base_string(long long val, int base, char *out_buf, size_t out_size) {
    if (val == 0) {
        snprintf(out_buf, out_size, "0");
        return;
    }

    bool negative = (val < 0);
    unsigned long long uval = negative ? (unsigned long long)(-(val + 1)) + 1ULL : (unsigned long long)val;

    char temp[64];
    int idx = 0;
    while (uval > 0 && idx < (int)sizeof(temp) - 1) {
        int rem = (int)(uval % (unsigned long long)base);
        temp[idx++] = (char)(rem < 10 ? ('0' + rem) : ('A' + rem - 10));
        uval /= (unsigned long long)base;
    }

    int dest = 0;
    if (negative && dest < (int)out_size - 1) out_buf[dest++] = '-';

    for (int i = idx - 1; i >= 0 && dest < (int)out_size - 1; i--) {
        out_buf[dest++] = temp[i];
    }
    out_buf[dest] = '\0';
}

static void strip_leading_zeros(const char *str, char *out_buf, size_t out_size) {
    if (str == NULL) return;
    size_t start = 0;
    bool negative = false;

    if (str[0] == '-') { negative = true; start = 1; }
    else if (str[0] == '+') { start = 1; }

    while (str[start] == '0' && str[start + 1] != '\0') start++;

    size_t dest = 0;
    if (negative && dest < out_size - 1) out_buf[dest++] = '-';
    while (str[start] != '\0' && dest < out_size - 1) {
        out_buf[dest++] = str[start++];
    }
    out_buf[dest] = '\0';
}

int main(void) {
    char *line = NULL;
    printf("Введите основание системы счисления [2..36]: ");
    status_code st = read_line(&line);
    if (st != SUCCESS || line == NULL) {
        fprintf(stderr, "Ошибка ввода.\n");
        free(line);
        return st == SUCCESS ? ERROR_INVALID_INPUT : st;
    }

    int base = 0;
    if (parse_int(line, &base) != SUCCESS || base < 2 || base > 36) {
        fprintf(stderr, "Ошибка: неверное основание системы счисления.\n");
        free(line);
        return ERROR_INVALID_INPUT;
    }
    free(line);
    line = NULL;

    printf("Введите числа (для окончания введите 'Stop'):\n");

    bool has_numbers = false;
    long long sum = 0;
    unsigned long long max_abs_val = 0;
    char *max_abs_str = NULL;

    while (true) {
        st = read_line(&line);
        if (st != SUCCESS) {
            fprintf(stderr, "Ошибка выделения памяти.\n");
            free(line);
            free(max_abs_str);
            return st;
        }
        if (line == NULL) break;

        if (strcmp(line, "Stop") == 0) {
            free(line);
            line = NULL;
            break;
        }

        long long val = 0;
        if (parse_custom_number(line, base, &val) != SUCCESS) {
            fprintf(stderr, "Предупреждение: пропущено невалидное число '%s'\n", line);
            free(line);
            line = NULL;
            continue;
        }

        unsigned long long abs_val = (val < 0) ? (unsigned long long)(-(val + 1)) + 1ULL : (unsigned long long)val;

        if (!has_numbers) {
            sum = val;
            max_abs_val = abs_val;
            max_abs_str = clone_string(line);
            if (max_abs_str == NULL) {
                fprintf(stderr, "Ошибка памяти.\n");
                free(line);
                return ERROR_MEMORY_ALLOCATION;
            }
            has_numbers = true;
        } else {
            if (__builtin_add_overflow(sum, val, &sum)) {
                fprintf(stderr, "Ошибка: переполнение при сложении.\n");
                free(line);
                free(max_abs_str);
                return ERROR_OVERFLOW;
            }
            if (abs_val > max_abs_val) {
                max_abs_val = abs_val;
                free(max_abs_str);
                max_abs_str = clone_string(line);
                if (max_abs_str == NULL) {
                    fprintf(stderr, "Ошибка памяти.\n");
                    free(line);
                    return ERROR_MEMORY_ALLOCATION;
                }
            }
        }

        free(line);
        line = NULL;
    }

    if (!has_numbers) {
        printf("Не было введено ни одного корректного числа.\n");
        return SUCCESS;
    }

    char stripped_max[256];
    strip_leading_zeros(max_abs_str, stripped_max, sizeof(stripped_max));

    char stripped_sum[256];
    char sum_orig[256];
    to_base_string(sum, base, sum_orig, sizeof(sum_orig));
    strip_leading_zeros(sum_orig, stripped_sum, sizeof(stripped_sum));

    printf("\n--- Результаты ---\n");
    printf("Максимальное по модулю число: %s\n", stripped_max);
    int bases[] = {9, 18, 27, 36};
    for (int i = 0; i < 4; i++) {
        char conv[256];
        to_base_string((long long)max_abs_val, bases[i], conv, sizeof(conv));
        printf("  В базе %d: %s\n", bases[i], conv);
    }

    printf("\nСумма всех чисел: %s\n", stripped_sum);
    for (int i = 0; i < 4; i++) {
        char conv[256];
        to_base_string(sum, bases[i], conv, sizeof(conv));
        printf("  В базе %d: %s\n", bases[i], conv);
    }

    free(max_abs_str);
    return SUCCESS;
}