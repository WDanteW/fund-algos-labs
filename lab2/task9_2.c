#include <stdio.h>
#include <stdarg.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_PARAM
} status_code;

static long long gcd(long long a, long long b) {
    while (b != 0) {
        long long temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

static int has_finite_representation(double num, int base) {
    if (num <= 0.0 || num >= 1.0 || base < 2 || base > 36) {
        return 0;
    }

    long long numerator = 0;
    long long denominator = 1000000000LL;
    double temp = num * (double)denominator;
    numerator = (long long)(temp + 0.5);

    long long common = gcd(numerator, denominator);
    numerator /= common;
    denominator /= common;

    while (denominator % 2 == 0 && base % 2 == 0) {
        denominator /= 2;
    }
    while (denominator % 5 == 0 && base % 5 == 0) {
        denominator /= 5;
    }

    for (int i = 3; i <= base; i += 2) {
        int prime = 1;
        for (int j = 2; j * j <= i; ++j) {
            if (i % j == 0) {
                prime = 0;
                break;
            }
        }
        if (prime && base % i == 0) {
            while (denominator % i == 0) {
                denominator /= i;
            }
        }
    }

    return denominator == 1;
}

static status_code check_fractions(int base, int count, ...) {
    if (base < 2 || base > 36 || count <= 0) {
        return ERROR_INVALID_PARAM;
    }

    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; ++i) {
        double val = va_arg(args, double);
        int finite = has_finite_representation(val, base);
        if (finite) {
            printf("Fraction %.6f in base %d: FINITE\n", val, base);
        } else {
            printf("Fraction %.6f in base %d: INFINITE\n", val, base);
        }
    }

    va_end(args);
    return SUCCESS;
}

int main(void) {
    check_fractions(2, 3, 0.5, 0.25, 0.1);
    check_fractions(10, 3, 0.5, 0.2, 0);

    return SUCCESS;
}