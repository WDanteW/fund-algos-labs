#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION
} status_code;

static size_t my_strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

static char digit_to_char(int digit, int uppercase) {
    if (digit >= 0 && digit <= 9) {
        return (char)('0' + digit);
    }
    return (char)((uppercase ? 'A' : 'a') + (digit - 10));
}

static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return -1;
}

static char *add_two_numbers(const char *num1, const char *num2, int base) {
    if (base < 2 || base > 36) {
        base = 10;
    }

    size_t len1 = my_strlen(num1);
    size_t len2 = my_strlen(num2);
    size_t max_len = (len1 > len2) ? len1 : len2;

    char *res = (char *)malloc(max_len + 2);
    if (res == NULL) {
        return NULL;
    }

    int carry = 0;
    size_t idx = 0;
    long long i = (long long)len1 - 1;
    long long j = (long long)len2 - 1;

    while (i >= 0 || j >= 0 || carry > 0) {
        int d1 = (i >= 0) ? char_to_digit(num1[i]) : 0;
        int d2 = (j >= 0) ? char_to_digit(num2[j]) : 0;

        int sum = d1 + d2 + carry;
        carry = sum / base;
        int digit = sum % base;

        res[idx++] = digit_to_char(digit, 0);

        if (i >= 0) {
            i--;
        }
        if (j >= 0) {
            j--;
        }
    }

    res[idx] = '\0';

    for (size_t k = 0; k < idx / 2; ++k) {
        char temp = res[k];
        res[k] = res[idx - 1 - k];
        res[idx - 1 - k] = temp;
    }

    size_t start = 0;
    while (res[start] == '0' && res[start + 1] != '\0') {
        start++;
    }

    if (start > 0) {
        size_t new_len = idx - start;
        for (size_t k = 0; k <= new_len; ++k) {
            res[k] = res[start + k];
        }
    }

    return res;
}

static status_code sum_in_base(int base, int count, char **result, ...) {
    if (base < 2 || base > 36 || count <= 0 || result == NULL) {
        return ERROR_INVALID_PARAM;
    }

    va_list args;
    va_start(args, result);

    char *current_sum = (char *)malloc(2);
    if (current_sum == NULL) {
        va_end(args);
        return ERROR_MEMORY_ALLOCATION;
    }
    current_sum[0] = '0';
    current_sum[1] = '\0';

    for (int i = 0; i < count; ++i) {
        const char *next_num = va_arg(args, const char *);
        char *temp = add_two_numbers(current_sum, next_num, base);
        free(current_sum);
        if (temp == NULL) {
            va_end(args);
            return ERROR_MEMORY_ALLOCATION;
        }
        current_sum = temp;
    }

    va_end(args);
    *result = current_sum;
    return SUCCESS;
}

int main(void) {
    char *result = NULL;

    status_code status = sum_in_base(16, 3, &result, "a", "f", "10");
    if (status == SUCCESS) {
        printf("Sum in base 16: %s\n", result);
        free(result);
    }

    status = sum_in_base(2, 4, &result, "10", "1101", "111", "1");
    if (status == SUCCESS) {
        printf("Sum in base 2: %s\n", result);
        free(result);
    }

    status = sum_in_base(16, 2, &result, "FFFFFFFFFFFFFFFF", "1");
    if (status == SUCCESS) {
        printf("Large hex sum: %s\n", result);
        free(result);
    }

    return SUCCESS;
}