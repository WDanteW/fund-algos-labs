#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_MEMORY_ALLOCATION,
    ERROR_INVALID_INPUT,
    ERROR_ZERO_DIVISION
} status_code;

typedef struct point {
    double x;
    double y;
} point;

static double cross_product_z(point a, point b, point c) {
    return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
}

static status_code is_convex_polygon(int count, bool *result, ...) {
    if (count < 3 || result == NULL) {
        return ERROR_INVALID_ARG_COUNT;
    }

    point *pts = (point *)malloc((size_t)count * sizeof(point));
    if (pts == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    va_list args;
    va_start(args, result);
    for (int i = 0; i < count; i++) {
        pts[i].x = va_arg(args, double);
        pts[i].y = va_arg(args, double);
    }
    va_end(args);

    bool has_pos = false;
    bool has_neg = false;
    bool convex = true;

    for (int i = 0; i < count; i++) {
        point a = pts[i];
        point b = pts[(i + 1) % count];
        point c = pts[(i + 2) % count];

        double cp = cross_product_z(a, b, c);
        if (cp > 1e-9) {
            has_pos = true;
        } else if (cp < -1e-9) {
            has_neg = true;
        }

        if (has_pos && has_neg) {
            convex = false;
            break;
        }
    }

    free(pts);
    *result = convex;
    return SUCCESS;
}

static status_code evaluate_polynomial(double x, int degree, double *result, ...) {
    if (degree < 0 || result == NULL) {
        return ERROR_INVALID_ARG_COUNT;
    }

    va_list args;
    va_start(args, result);

    double sum = 0.0;
    for (int i = 0; i <= degree; i++) {
        double coef = va_arg(args, double);
        sum = sum * x + coef;
    }

    va_end(args);
    *result = sum;
    return SUCCESS;
}

static bool is_kaprekar(unsigned long long num, int base) {
    if (num == 0) return false;
    if (num == 1) return true;

    unsigned long long sq = num * num;
    unsigned long long temp = sq;
    unsigned long long divisor = 1;

    while (temp > 0) {
        divisor *= (unsigned long long)base;
        temp /= (unsigned long long)base;
    }

    divisor /= (unsigned long long)base;

    while (divisor > 1) {
        unsigned long long right = sq % divisor;
        unsigned long long left = sq / divisor;
        if (right > 0 && left + right == num) {
            return true;
        }
        divisor /= (unsigned long long)base;
    }

    return false;
}

static status_code find_kaprekar_numbers(int base, int count, void (*callback)(const char *), ...) {
    if (base < 2 || count < 0 || callback == NULL) {
        return ERROR_INVALID_INPUT;
    }

    va_list args;
    va_start(args, callback);

    for (int i = 0; i < count; i++) {
        const char *str_num = va_arg(args, const char *);
        if (str_num == NULL) continue;

        char *endptr = NULL;
        unsigned long long val = strtoull(str_num, &endptr, base);

        if (endptr == str_num || *endptr != '\0') {
            continue;
        }

        if (is_kaprekar(val, base)) {
            callback(str_num);
        }
    }

    va_end(args);
    return SUCCESS;
}

static status_code geometric_mean(int count, double *result, ...) {
    if (count <= 0 || result == NULL) {
        return ERROR_INVALID_ARG_COUNT;
    }

    va_list args;
    va_start(args, result);

    double log_sum = 0.0;
    for (int i = 0; i < count; i++) {
        double val = va_arg(args, double);
        if (val <= 0.0) {
            va_end(args);
            return ERROR_INVALID_INPUT;
        }
        log_sum += log(val);
    }

    va_end(args);
    *result = exp(log_sum / (double)count);
    return SUCCESS;
}

static double power_fast(double base, long long exp) {
    if (exp == 0) return 1.0;
    if (exp < 0) {
        return 1.0 / power_fast(base, -exp);
    }
    double half = power_fast(base, exp / 2);
    if (exp % 2 == 0) {
        return half * half;
    } else {
        return base * half * half;
    }
}

static status_code solve_bisection(double a, double b, double eps, double (*func)(double), double *root) {
    if (func == NULL || root == NULL || eps <= 0.0) {
        return ERROR_INVALID_INPUT;
    }

    double fa = func(a);
    double fb = func(b);

    if (fa * fb > 0.0) {
        return ERROR_INVALID_INPUT;
    }

    double left = a;
    double right = b;
    double mid = left;

    while ((right - left) >= eps) {
        mid = left + (right - left) / 2.0;
        double fmid = func(mid);

        if (fabs(fmid) < 1e-14) {
            break;
        }

        if (fa * fmid < 0.0) {
            right = mid;
            fb = fmid;
        } else {
            left = mid;
            fa = fmid;
        }
    }

    *root = mid;
    return SUCCESS;
}

static void print_callback(const char *str) {
    printf("  Найдено число Капрекара: %s\n", str);
}

static double sample_eq1(double x) {
    return x * x - 4.0;
}

static double sample_eq2(double x) {
    return sin(x);
}

int main(void) {
    printf("--- Запуск демонстрации задачи 6 ---\n\n");

    bool is_convex = false;
    is_convex_polygon(4, &is_convex, 0.0, 0.0, 2.0, 0.0, 2.0, 2.0, 0.0, 2.0);
    printf("1. Многоугольник выпуклый: %s\n\n", is_convex ? "true" : "false");

    double poly_res = 0.0;
    evaluate_polynomial(2.0, 3, &poly_res, 1.0, -3.0, 2.0, -5.0);
    printf("2. Значение многочлена в точке 2.0: %.4f\n\n", poly_res);

    printf("3. Поиск чисел Капрекара (база 10):\n");
    find_kaprekar_numbers(10, 3, print_callback, "45", "9", "55");
    printf("\n");

    double gm_res = 0.0;
    geometric_mean(3, &gm_res, 2.0, 8.0, 32.0);
    printf("4. Среднее геометрическое (2, 8, 32): %.4f\n\n", gm_res);

    double pow_res = power_fast(2.0, -3);
    printf("5. Быстрое возведение в степень (2^-3): %.6f\n\n", pow_res);

    double root1 = 0.0, root2 = 0.0;
    solve_bisection(0.0, 5.0, 1e-5, sample_eq1, &root1);
    solve_bisection(3.0, 4.0, 1e-5, sample_eq2, &root2);
    printf("6. Корни уравнений:\n");
    printf("   Уравнение x^2 - 4 = 0 на [0, 5]: x = %.5f\n", root1);
    printf("   Уравнение sin(x) = 0 на [3, 4]: x = %.5f\n", root2);

    return SUCCESS;
}