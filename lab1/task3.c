#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <errno.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_FLAG,
    ERROR_INVALID_NUMBER,
    ERROR_ZERO_DIVISION
} status_code;

static status_code parse_double(const char *str, double *result) {
    if (str == NULL || result == NULL || *str == '\0') {
        return ERROR_INVALID_NUMBER;
    }

    char *endptr = NULL;
    errno = 0;
    double val = strtod(str, &endptr);

    if (errno == ERANGE || *endptr != '\0') {
        return ERROR_INVALID_NUMBER;
    }

    *result = val;
    return SUCCESS;
}

static status_code parse_long(const char *str, long *result) {
    if (str == NULL || result == NULL || *str == '\0') {
        return ERROR_INVALID_NUMBER;
    }

    char *endptr = NULL;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE || *endptr != '\0') {
        return ERROR_INVALID_NUMBER;
    }

    *result = val;
    return SUCCESS;
}

static bool doubles_equal(double a, double b, double eps) {
    return fabs(a - b) < eps;
}

static void solve_quadratic(double a, double b, double c, double eps) {
    printf("Уравнение: (%.4f)*x^2 + (%.4f)*x + (%.4f) = 0\n", a, b, c);

    if (doubles_equal(a, 0.0, eps)) {
        if (doubles_equal(b, 0.0, eps)) {
            if (doubles_equal(c, 0.0, eps)) {
                printf("  Бесконечно много решений.\n\n");
            } else {
                printf("  Нет решений.\n\n");
            }
        } else {
            double x = -c / b;
            printf("  Линейное уравнение, корень: x = %.6f\n\n", x);
        }
        return;
    }

    double disc = b * b - 4.0 * a * c;

    if (disc > eps) {
        double x1 = (-b + sqrt(disc)) / (2.0 * a);
        double x2 = (-b - sqrt(disc)) / (2.0 * a);
        printf("  Два корня: x1 = %.6f, x2 = %.6f\n\n", x1, x2);
    } else if (fabs(disc) <= eps) {
        double x = -b / (2.0 * a);
        printf("  Один корень: x = %.6f\n\n", x);
    } else {
        printf("  Нет действительных корней.\n\n");
    }
}

static status_code process_q_flag(double eps, double a, double b, double c) {
    if (eps <= 0.0 || eps >= 1.0) {
        return ERROR_INVALID_NUMBER;
    }

    double coefs[3] = {a, b, c};

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (j == i) continue;
            for (int k = 0; k < 3; k++) {
                if (k == i || k == j) continue;

                if (i > j && doubles_equal(coefs[i], coefs[j], eps)) continue;
                if (i > k && doubles_equal(coefs[i], coefs[k], eps)) continue;
                if (j > k && doubles_equal(coefs[j], coefs[k], eps)) continue;

                solve_quadratic(coefs[i], coefs[j], coefs[k], eps);
            }
        }
    }

    return SUCCESS;
}

static status_code process_m_flag(long num1, long num2) {
    if (num1 == 0 || num2 == 0) {
        return ERROR_ZERO_DIVISION;
    }

    if (num1 % num2 == 0) {
        printf("Число %ld кратно числу %ld\n", num1, num2);
    } else {
        printf("Число %ld НЕ кратно числу %ld\n", num1, num2);
    }

    return SUCCESS;
}

static status_code process_t_flag(double eps, double a, double b, double c) {
    if (eps <= 0.0 || eps >= 1.0 || a <= 0.0 || b <= 0.0 || c <= 0.0) {
        return ERROR_INVALID_NUMBER;
    }

    bool is_right = doubles_equal(a * a + b * b, c * c, eps) ||
                    doubles_equal(a * a + c * c, b * b, eps) ||
                    doubles_equal(b * b + c * c, a * a, eps);

    if (is_right) {
        printf("Числа %.4f, %.4f, %.4f МОГУТ являться сторонами прямоугольного треугольника.\n", a, b, c);
    } else {
        printf("Числа %.4f, %.4f, %.4f НЕ могут являться сторонами прямоугольного треугольника.\n", a, b, c);
    }

    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка: недопустимое количество аргументов.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *flag = argv[1];
    if ((flag[0] != '-' && flag[0] != '/') || flag[1] == '\0' || flag[2] != '\0') {
        fprintf(stderr, "Ошибка: неверный формат флага.\n");
        return ERROR_INVALID_FLAG;
    }

    char flag_char = flag[1];

    if (flag_char == 'q') {
        if (argc != 6) {
            fprintf(stderr, "Ошибка: флаг -q требует 4 параметра: <eps> <a> <b> <c>\n");
            return ERROR_INVALID_ARG_COUNT;
        }

        double eps = 0.0, a = 0.0, b = 0.0, c = 0.0;
        if (parse_double(argv[2], &eps) != SUCCESS ||
            parse_double(argv[3], &a) != SUCCESS ||
            parse_double(argv[4], &b) != SUCCESS ||
            parse_double(argv[5], &c) != SUCCESS) {
            fprintf(stderr, "Ошибка: невалидные числовые аргументы для флага -q.\n");
            return ERROR_INVALID_NUMBER;
        }

        return process_q_flag(eps, a, b, c);

    } else if (flag_char == 'm') {
        if (argc != 4) {
            fprintf(stderr, "Ошибка: флаг -m требует 2 параметра: <num1> <num2>\n");
            return ERROR_INVALID_ARG_COUNT;
        }

        long num1 = 0, num2 = 0;
        if (parse_long(argv[2], &num1) != SUCCESS ||
            parse_long(argv[3], &num2) != SUCCESS) {
            fprintf(stderr, "Ошибка: невалидные целые числа для флага -m.\n");
            return ERROR_INVALID_NUMBER;
        }

        status_code st = process_m_flag(num1, num2);
        if (st == ERROR_ZERO_DIVISION) {
            fprintf(stderr, "Ошибка: числа не должны быть равны нулю.\n");
            return ERROR_ZERO_DIVISION;
        }

        return SUCCESS;

    } else if (flag_char == 't') {
        if (argc != 6) {
            fprintf(stderr, "Ошибка: флаг -t требует 4 параметра: <eps> <a> <b> <c>\n");
            return ERROR_INVALID_ARG_COUNT;
        }

        double eps = 0.0, a = 0.0, b = 0.0, c = 0.0;
        if (parse_double(argv[2], &eps) != SUCCESS ||
            parse_double(argv[3], &a) != SUCCESS ||
            parse_double(argv[4], &b) != SUCCESS ||
            parse_double(argv[5], &c) != SUCCESS) {
            fprintf(stderr, "Ошибка: невалидные числовые аргументы для флага -t.\n");
            return ERROR_INVALID_NUMBER;
        }

        return process_t_flag(eps, a, b, c);

    } else {
        fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", flag);
        return ERROR_INVALID_FLAG;
    }
}