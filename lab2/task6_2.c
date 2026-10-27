#include <stdio.h>
#include <stdarg.h>
#include <string.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION,
    ERROR_FILE_OPERATION
} status_code;

static int parse_roman_char(char c) {
    switch (c) {
        case 'I': return 1;
        case 'V': return 5;
        case 'X': return 10;
        case 'L': return 50;
        case 'C': return 100;
        case 'D': return 500;
        case 'M': return 1000;
        default: return 0;
    }
}

static int parse_roman(const char *s, int *val, size_t *consumed) {
    int total = 0;
    int prev_val = 0;
    size_t len = 0;
    while (s[len] != '\0' && s[len] != ' ' && s[len] != '\t' && s[len] != '\n' && s[len] != '\r') {
        len++;
    }
    if (len == 0) {
        return 0;
    }

    for (int idx = (int)len - 1; idx >= 0; --idx) {
        int cur_val = parse_roman_char(s[idx]);
        if (cur_val == 0) {
            return 0;
        }
        if (cur_val < prev_val) {
            total -= cur_val;
        } else {
            total += cur_val;
            prev_val = cur_val;
        }
    }

    *val = total;
    *consumed = len;
    return 1;
}

static int parse_zeckendorf(const char *s, unsigned int *val, size_t *consumed) {
    size_t len = 0;
    while (s[len] != '\0' && s[len] != ' ' && s[len] != '\t' && s[len] != '\n' && s[len] != '\r') {
        len++;
    }
    if (len < 2 || s[len - 1] != '1') {
        return 0;
    }

    for (size_t idx = 0; idx < len; ++idx) {
        if (s[idx] != '0' && s[idx] != '1') {
            return 0;
        }
    }

    unsigned long long fib[90];
    fib[0] = 1;
    fib[1] = 2;
    int count = 2;
    while (count < 90 && (size_t)count < len - 1) {
        fib[count] = fib[count - 1] + fib[count - 2];
        count++;
    }

    unsigned long long sum = 0;
    for (size_t idx = 0; idx < len - 1; ++idx) {
        if (s[idx] == '1') {
            if (idx >= (size_t)count) {
                return 0;
            }
            sum += fib[idx];
        }
    }

    *val = (unsigned int)sum;
    *consumed = len;
    return 1;
}

static int parse_custom_base_str(const char *s, int base, int uppercase, int *val, size_t *consumed) {
    if (base < 2 || base > 36) {
        base = 10;
    }
    size_t idx = 0;
    long long res = 0;
    int sign = 1;

    if (s[0] == '-') {
        sign = -1;
        idx++;
    } else if (s[0] == '+') {
        idx++;
    }

    size_t start = idx;
    while (s[idx] != '\0' && s[idx] != ' ' && s[idx] != '\t' && s[idx] != '\n' && s[idx] != '\r') {
        char c = s[idx];
        int digit = -1;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (uppercase && c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else if (!uppercase && c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        }
        if (digit < 0 || digit >= base) {
            break;
        }
        res = res * base + digit;
        idx++;
    }

    if (idx == start) {
        return 0;
    }

    *val = (int)(res * sign);
    *consumed = idx;
    return 1;
}

static int v_sscanf_custom(const char *str, const char *format, va_list args) {
    int assigned = 0;
    const char *p = format;
    const char *s = str;

    while (*p != '\0') {
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
            s++;
        }
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
            continue;
        }

        if (*p != '%') {
            if (*s != *p) {
                break;
            }
            s++;
            p++;
        } else {
            p++;
            if (*p == 'R' && *(p + 1) == 'o') {
                p += 2;
                int *dest = va_arg(args, int *);
                size_t consumed = 0;
                if (!parse_roman(s, dest, &consumed)) {
                    break;
                }
                s += consumed;
                assigned++;
            } else if (*p == 'Z' && *(p + 1) == 'r') {
                p += 2;
                unsigned int *dest = va_arg(args, unsigned int *);
                size_t consumed = 0;
                if (!parse_zeckendorf(s, dest, &consumed)) {
                    break;
                }
                s += consumed;
                assigned++;
            } else if (*p == 'C' && (*(p + 1) == 'v' || *(p + 1) == 'V')) {
                int upper = (*(p + 1) == 'V');
                p += 2;
                int *dest = va_arg(args, int *);
                int base = va_arg(args, int);
                size_t consumed = 0;
                if (!parse_custom_base_str(s, base, upper, dest, &consumed)) {
                    break;
                }
                s += consumed;
                assigned++;
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

                if (strstr(spec_buf, "s") != NULL) {
                    char *dest = va_arg(args, char *);
                    if (sscanf(s, spec_buf, dest) != 1) {
                        break;
                    }
                    while (*s != '\0' && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') {
                        s++;
                    }
                } else {
                    int *dest = va_arg(args, int *);
                    if (sscanf(s, spec_buf, dest) != 1) {
                        break;
                    }
                    while (*s != '\0' && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') {
                        s++;
                    }
                }
                assigned++;
            }
        }
    }

    return assigned;
}

static int oversscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int res = v_sscanf_custom(str, format, args);
    va_end(args);
    return res;
}

static int overfscanf(FILE *stream, const char *format, ...) {
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stream) == NULL) {
        return -1;
    }
    va_list args;
    va_start(args, format);
    int res = v_sscanf_custom(buffer, format, args);
    va_end(args);
    return res;
}

int main(void) {
    int roman_val = 0;
    unsigned int zeck_val = 0;
    int custom_val = 0;

    oversscanf("XI 1011 fa", "%Ro %Zr %Cv", &roman_val, &zeck_val, &custom_val, 16);
    printf("Roman: %d, Zeckendorf: %u, Custom: %d\n", roman_val, zeck_val, custom_val);

    overfscanf(stdout, "%Ro", &roman_val);

    return SUCCESS;
}