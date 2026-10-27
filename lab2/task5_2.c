#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION,
    ERROR_FILE_OPERATION
} status_code;

static int append_to_buffer(char **str, size_t *size, size_t *capacity, const char *to_add, size_t add_len) {
    if (*size + add_len + 1 >= *capacity) {
        *capacity = (*capacity * 2 > *size + add_len + 1) ? *capacity * 2 : (*size + add_len + 1 + 64);
        char *temp = (char *)realloc(*str, *capacity);
        if (temp == NULL) {
            return 0;
        }
        *str = temp;
    }
    for (size_t i = 0; i < add_len; ++i) {
        (*str)[*size + i] = to_add[i];
    }
    *size += add_len;
    (*str)[*size] = '\0';
    return 1;
}

static int format_roman(int val, char **str, size_t *size, size_t *capacity) {
    if (val <= 0 || val > 3999) {
        const char *err = "ROMAN_ERR";
        return append_to_buffer(str, size, capacity, err, 9);
    }

    struct {
        int value;
        const char *roman;
    } rules[] = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"},
        {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"}
    };

    char buf[128] = {0};
    size_t len = 0;

    int n = val;
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); ++i) {
        while (n >= rules[i].value) {
            const char *r = rules[i].roman;
            while (*r != '\0') {
                buf[len++] = *r;
                r++;
            }
            n -= rules[i].value;
        }
    }

    return append_to_buffer(str, size, capacity, buf, len);
}

static int format_zeckendorf(unsigned int val, char **str, size_t *size, size_t *capacity) {
    if (val == 0) {
        const char *res = "01";
        return append_to_buffer(str, size, capacity, res, 2);
    }

    unsigned long long fib[90];
    fib[0] = 1;
    fib[1] = 2;
    int count = 2;
    while (count < 90 && fib[count - 1] + fib[count - 2] <= val) {
        fib[count] = fib[count - 1] + fib[count - 2];
        count++;
    }

    char buf[128];
    int idx = 0;
    unsigned long long rem = val;

    for (int i = count - 1; i >= 0; --i) {
        if (rem >= fib[i]) {
            buf[idx++] = '1';
            rem -= fib[i];
        } else {
            buf[idx++] = '0';
        }
    }

    buf[idx++] = '1';
    return append_to_buffer(str, size, capacity, buf, (size_t)idx);
}

static int format_custom_base(long long val, int base, int uppercase, char **str, size_t *size, size_t *capacity) {
    if (base < 2 || base > 36) {
        base = 10;
    }

    char buf[128];
    int idx = 0;
    int is_neg = 0;
    unsigned long long uval = 0;

    if (val < 0 && base == 10) {
        is_neg = 1;
        uval = (unsigned long long)(-val);
    } else {
        uval = (unsigned long long)val;
    }

    if (uval == 0) {
        buf[idx++] = '0';
    } else {
        const char *digits = uppercase ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";
        while (uval > 0) {
            buf[idx++] = digits[uval % (unsigned long long)base];
            uval /= (unsigned long long)base;
        }
    }

    if (is_neg) {
        buf[idx++] = '-';
    }

    char rev[128];
    for (int i = 0; i < idx; ++i) {
        rev[i] = buf[idx - 1 - i];
    }

    return append_to_buffer(str, size, capacity, rev, (size_t)idx);
}

static int parse_custom_base(const char *s, int base, int uppercase) {
    if (base < 2 || base > 36) {
        base = 10;
    }
    long long res = 0;
    int sign = 1;
    size_t i = 0;

    if (s[0] == '-') {
        sign = -1;
        i++;
    } else if (s[0] == '+') {
        i++;
    }

    while (s[i] != '\0') {
        char c = s[i];
        int val = -1;
        if (c >= '0' && c <= '9') {
            val = c - '0';
        } else if (uppercase && c >= 'A' && c <= 'Z') {
            val = c - 'A' + 10;
        } else if (!uppercase && c >= 'a' && c <= 'z') {
            val = c - 'a' + 10;
        }
        if (val >= base || val < 0) {
            break;
        }
        res = res * base + val;
        i++;
    }

    return (int)(res * sign);
}

static int format_memory_dump(const void *ptr, size_t size, char **str, size_t *sz, size_t *capacity) {
    const unsigned char *bytes = (const unsigned char *)ptr;
    char buf[128];
    size_t len = 0;

    for (size_t i = 0; i < size; ++i) {
        unsigned char b = bytes[i];
        for (int bit = 7; bit >= 0; --bit) {
            buf[len++] = ((b >> bit) & 1) ? '1' : '0';
        }
        if (i + 1 < size) {
            buf[len++] = ' ';
        }
    }

    return append_to_buffer(str, sz, capacity, buf, len);
}

static int vvasprintf(char **result_str, const char *format, va_list args) {
    size_t capacity = 128;
    size_t size = 0;
    char *str = (char *)malloc(capacity);
    if (str == NULL) {
        return -1;
    }
    str[0] = '\0';

    const char *p = format;
    while (*p != '\0') {
        if (*p != '%') {
            if (!append_to_buffer(&str, &size, &capacity, p, 1)) {
                free(str);
                return -1;
            }
            p++;
        } else {
            p++;
            if (*p == '%') {
                if (!append_to_buffer(&str, &size, &capacity, "%", 1)) {
                    free(str);
                    return -1;
                }
                p++;
            } else if (*p == 'R' && *(p + 1) == 'o') {
                p += 2;
                int val = va_arg(args, int);
                if (!format_roman(val, &str, &size, &capacity)) {
                    free(str);
                    return -1;
                }
            } else if (*p == 'Z' && *(p + 1) == 'r') {
                p += 2;
                unsigned int val = va_arg(args, unsigned int);
                if (!format_zeckendorf(val, &str, &size, &capacity)) {
                    free(str);
                    return -1;
                }
            } else if (*p == 'C' && (*(p + 1) == 'v' || *(p + 1) == 'V')) {
                int upper = (*(p + 1) == 'V');
                p += 2;
                int val = va_arg(args, int);
                int base = va_arg(args, int);
                if (!format_custom_base(val, base, upper, &str, &size, &capacity)) {
                    free(str);
                    return -1;
                }
            } else if (*p == 't' && (*(p + 1) == 'o' || *(p + 1) == 'O')) {
                int upper = (*(p + 1) == 'O');
                p += 2;
                const char *sval = va_arg(args, const char *);
                int base = va_arg(args, int);
                int converted = parse_custom_base(sval, base, upper);
                if (!format_custom_base(converted, 10, 0, &str, &size, &capacity)) {
                    free(str);
                    return -1;
                }
            } else if (*p == 'm' && (*(p + 1) == 'i' || *(p + 1) == 'u' || *(p + 1) == 'd' || *(p + 1) == 'f')) {
                char sub = *(p + 1);
                p += 2;
                if (sub == 'i') {
                    int val = va_arg(args, int);
                    if (!format_memory_dump(&val, sizeof(val), &str, &size, &capacity)) {
                        free(str);
                        return -1;
                    }
                } else if (sub == 'u') {
                    unsigned int val = va_arg(args, unsigned int);
                    if (!format_memory_dump(&val, sizeof(val), &str, &size, &capacity)) {
                        free(str);
                        return -1;
                    }
                } else if (sub == 'd') {
                    double val = va_arg(args, double);
                    if (!format_memory_dump(&val, sizeof(val), &str, &size, &capacity)) {
                        free(str);
                        return -1;
                    }
                } else if (sub == 'f') {
                    float val = (float)va_arg(args, double);
                    if (!format_memory_dump(&val, sizeof(val), &str, &size, &capacity)) {
                        free(str);
                        return -1;
                    }
                }
            } else {
                char spec_buf[64];
                size_t slen = 0;
                spec_buf[slen++] = '%';
                while (*p != '\0' && *p != 'd' && *p != 'i' && *p != 'u' && *p != 'x' && *p != 'X' && *p != 's' && *p != 'c' && *p != 'f' && *p != 'p') {
                    spec_buf[slen++] = *p;
                    p++;
                }
                if (*p != '\0') {
                    spec_buf[slen++] = *p;
                    p++;
                }
                spec_buf[slen] = '\0';

                char temp_out[256];
                if (strstr(spec_buf, "s") != NULL) {
                    char *sval = va_arg(args, char *);
                    snprintf(temp_out, sizeof(temp_out), spec_buf, sval);
                } else if (strstr(spec_buf, "f") != NULL) {
                    double dval = va_arg(args, double);
                    snprintf(temp_out, sizeof(temp_out), spec_buf, dval);
                } else {
                    int ival = va_arg(args, int);
                    snprintf(temp_out, sizeof(temp_out), spec_buf, ival);
                }

                if (!append_to_buffer(&str, &size, &capacity, temp_out, strlen(temp_out))) {
                    free(str);
                    return -1;
                }
            }
        }
    }

    *result_str = str;
    return (int)size;
}

static int oversprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *res = NULL;
    int len = vvasprintf(&res, format, args);
    va_end(args);

    if (len < 0) {
        return -1;
    }
    for (int i = 0; i <= len; ++i) {
        str[i] = res[i];
    }
    free(res);
    return len;
}

static int overfprintf(FILE *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *res = NULL;
    int len = vvasprintf(&res, format, args);
    va_end(args);

    if (len < 0) {
        return -1;
    }
    int written = (int)fwrite(res, 1, (size_t)len, stream);
    free(res);
    return written;
}

int main(void) {
    char buffer[1024];

    oversprintf(buffer, "Roman: %Ro\n", 1999);
    printf("%s", buffer);

    oversprintf(buffer, "Zeckendorf: %Zr\n", 10);
    printf("%s", buffer);

    oversprintf(buffer, "Custom Base Cv: %Cv\n", 255, 16);
    printf("%s", buffer);

    oversprintf(buffer, "Custom Data CV: %CV\n", 255, 16);
    printf("%s", buffer);

    oversprintf(buffer, "Parse to base 10: %to\n", "ff", 16);
    printf("%s", buffer);

    oversprintf(buffer, "Memory Dump mi: %mi\n", -4);
    printf("%s", buffer);

    oversprintf(buffer, "Memory Dump mf: %mf\n", 3.14f);
    printf("%s", buffer);

    overfprintf(stdout, "Test overfprintf: %Ro\n", 22);

    return SUCCESS;
}