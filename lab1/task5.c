#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_EPSILON
} status_code;

static status_code parse_double(const char *str, double *result) {
    if (str == NULL || result == NULL || *str == '\0') return ERROR_INVALID_EPSILON;
    char *endptr = NULL;
    double val = strtod(str, &endptr);
    if (endptr == str || *endptr != '\0') return ERROR_INVALID_EPSILON;
    *result = val;
    return SUCCESS;
}

static status_code sum_a(double eps, double x, double *res) {
    double sum = 0.0;
    double term = 1.0;
    int n = 0;
    while (fabs(term) >= eps) {
        sum += term;
        n++;
        term *= x / n;
    }
    *res = sum;
    return SUCCESS;
}

static status_code sum_b(double eps, double x, double *res) {
    double sum = 0.0;
    double term = 1.0;
    int n = 0;
    while (fabs(term) >= eps) {
        sum += term;
        n++;
        term *= -(x * x) / ((2.0 * n - 1.0) * (2.0 * n));
    }
    *res = sum;
    return SUCCESS;
}

static status_code sum_c(double eps, double x, double *res) {
    double sum = 0.0;
    double term = 1.0;
    int n = 0;
    while (fabs(term) >= eps) {
        sum += term;
        n++;
        term *= (27.0 * n * n * n * x * x) / ((3.0 * n - 2.0) * (3.0 * n - 1.0) * (3.0 * n));
    }
    *res = sum;
    return SUCCESS;
}

static status_code sum_d(double eps, double x, double *res) {
    double sum = 0.0;
    double term = -(x * x) / 2.0;
    int n = 1;
    while (fabs(term) >= eps) {
        sum += term;
        n++;
        term *= -((2.0 * n - 2.0) * (2.0 * n - 3.0) * x * x) / ((2.0 * n - 1.0) * (2.0 * n));
    }
    *res = sum;
    return SUCCESS;
}

static double f_int_a(double x) {
    if (fabs(x) < 1e-12) return 1.0;
    return log(1.0 + x) / x;
}

static double f_int_b(double x) {
    return exp(-(x * x) / 2.0);
}

static double f_int_c(double x) {
    if (fabs(1.0 - x) < 1e-12) return 0.0;
    return log(1.0 / (1.0 - x));
}

static double f_int_d(double x) {
    if (fabs(x) < 1e-12) return 1.0;
    return pow(x, x);
}

static double integrate_rectangles(double (*func)(double), double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double x_mid = a + (i + 0.5) * h;
        sum += func(x_mid);
    }
    return sum * h;
}

static status_code integral_generic(double (*func)(double), double a, double b, double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    int n = 10;
    double prev = integrate_rectangles(func, a, b, n);
    n *= 2;
    double current = integrate_rectangles(func, a, b, n);

    while (fabs(current - prev) >= eps) {
        prev = current;
        n *= 2;
        current = integrate_rectangles(func, a, b, n);
    }

    *res = current;
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <epsilon>\n", argv[0]);
        return ERROR_INVALID_ARG_COUNT;
    }

    double eps = 0.0;
    status_code status = parse_double(argv[1], &eps);
    if (status != SUCCESS || eps <= 0.0 || eps >= 1.0) {
        fprintf(stderr, "Ошибка: невалидное значение epsilon (должно быть в интервале (0, 1)).\n");
        return ERROR_INVALID_EPSILON;
    }

    double x_val = 0.5;
    double res = 0.0;

    printf("Точность eps = %g, тестовое x = %.2f\n\n", eps, x_val);

    sum_a(eps, x_val, &res);
    printf("Сумма 1.a: %.10f\n", res);

    sum_b(eps, x_val, &res);
    printf("Сумма 1.b: %.10f\n", res);

    sum_c(eps, x_val, &res);
    printf("Сумма 1.c: %.10f\n", res);

    sum_d(eps, x_val, &res);
    printf("Сумма 1.d: %.10f\n\n", res);

    integral_generic(f_int_a, 0.0, 1.0, eps, &res);
    printf("Интеграл 2.a: %.10f\n", res);

    integral_generic(f_int_b, 0.0, 1.0, eps, &res);
    printf("Интеграл 2.b: %.10f\n", res);

    integral_generic(f_int_c, 0.0, 1.0, eps, &res);
    printf("Интеграл 2.c: %.10f\n", res);

    integral_generic(f_int_d, 0.0, 1.0, eps, &res);
    printf("Интеграл 2.d: %.10f\n", res);

    return SUCCESS;
}