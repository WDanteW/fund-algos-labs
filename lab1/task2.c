#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_EPSILON,
    ERROR_MEMORY_ALLOCATION,
    ERROR_CALCULATION_FAILED
} status_code;

static status_code parse_double(const char *str, double *result) {
    if (str == NULL || result == NULL || *str == '\0') return ERROR_INVALID_EPSILON;
    char *endptr = NULL;
    double val = strtod(str, &endptr);
    if (endptr == str || *endptr != '\0' || val <= 0.0) return ERROR_INVALID_EPSILON;
    *result = val;
    return SUCCESS;
}

static status_code e_limit(double eps, double *res) {
    double prev = 0.0;
    long long n = 1;
    double current = pow(1.0 + 1.0 / (double)n, (double)n);
    while (fabs(current - prev) >= eps && n < 1000000000LL) {
        prev = current;
        n *= 2;
        current = pow(1.0 + 1.0 / (double)n, (double)n);
    }
    *res = current;
    return SUCCESS;
}

static status_code e_series(double eps, double *res) {
    double sum = 1.0;
    double term = 1.0;
    int n = 1;
    while (term >= eps) {
        term /= n;
        sum += term;
        n++;
    }
    *res = sum;
    return SUCCESS;
}

static status_code e_equation(double eps, double *res) {
    double x = 2.0;
    double dx = 1.0;
    while (fabs(dx) >= eps) {
        dx = (log(x) - 1.0) * x;
        x -= dx;
    }
    *res = x;
    return SUCCESS;
}

static status_code pi_limit(double eps, double *res) {
    double prev = 0.0;
    long long n = 1;
    double log_n_fact = 0.0;
    double log_2n_fact = 0.0;
    double current = 0.0;
    while (n < 1000000) {
        log_n_fact += log((double)n);
        log_2n_fact += log((double)(2 * n - 1)) + log((double)(2 * n));
        double log_num = 4.0 * ((double)n * log(2.0) + log_n_fact);
        double log_den = log((double)n) + 2.0 * log_2n_fact;
        current = exp(log_num - log_den);
        if (fabs(current - prev) < eps && n > 2) break;
        prev = current;
        n++;
    }
    *res = current;
    return SUCCESS;
}

static status_code pi_series(double eps, double *res) {
    double sum = 0.0;
    double term = 1.0;
    int n = 1;
    int sign = 1;
    while (fabs(term) >= eps && n < 100000000) {
        sum += sign * term;
        term = 1.0 / (2.0 * n + 1.0);
        sign = -sign;
        n++;
    }
    *res = 4.0 * sum;
    return SUCCESS;
}

static status_code pi_equation(double eps, double *res) {
    double x = 3.0;
    double dx = 1.0;
    while (fabs(dx) >= eps) {
        dx = (cos(x) + 1.0) / (-sin(x));
        x -= dx;
    }
    *res = x;
    return SUCCESS;
}

static status_code ln2_limit(double eps, double *res) {
    double prev = 0.0;
    long long n = 1;
    double current = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
    while (fabs(current - prev) >= eps && n < 1000000000LL) {
        prev = current;
        n *= 2;
        current = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
    }
    *res = current;
    return SUCCESS;
}

static status_code ln2_series(double eps, double *res) {
    double sum = 0.0;
    double term = 1.0;
    int n = 1;
    for (n = 1; n < 100000000; n++) {
        sum += term;
        term = -term * (double)n / (double)(n + 1);
        if (fabs(term) < eps) break;
    }
    *res = sum;
    return SUCCESS;
}

static status_code ln2_equation(double eps, double *res) {
    double x = 0.5;
    double dx = 1.0;
    while (fabs(dx) >= eps) {
        dx = (exp(x) - 2.0) / exp(x);
        x -= dx;
    }
    *res = x;
    return SUCCESS;
}

static status_code sqrt2_limit(double eps, double *res) {
    double x = -0.5;
    double next = x - (x * x) / 2.0 + 1.0;
    while (fabs(next - x) >= eps) {
        x = next;
        next = x - (x * x) / 2.0 + 1.0;
    }
    *res = next;
    return SUCCESS;
}

static status_code sqrt2_series(double eps, double *res) {
    double log_sum = 0.0;
    double term = 1.0 / 4.0;
    int k = 2;
    while (term >= eps) {
        log_sum += term * log(2.0);
        k++;
        term /= 2.0;
    }
    *res = exp(log_sum);
    return SUCCESS;
}

static status_code sqrt2_equation(double eps, double *res) {
    double x = 1.0;
    double dx = 1.0;
    while (fabs(dx) >= eps) {
        dx = (x * x - 2.0) / (2.0 * x);
        x -= dx;
    }
    *res = x;
    return SUCCESS;
}

static status_code gamma_limit(double eps, double *res) {
    long long n = 1;
    double prev = 1.0 - log(1.0);
    double current = prev;
    while (n < 1000000000LL) {
        n *= 2;
        double s = 0.0;
        for (long long k = 1; k <= n; ++k) s += 1.0 / (double)k;
        current = s - log((double)n);
        if (fabs(current - prev) < eps) break;
        prev = current;
    }
    *res = current;
    return SUCCESS;
}

static double zeta(double s) {
    if (s <= 1.0) return 0.0;
    const int N = 100000;
    double sum = 0.0;
    for (int n = 1; n <= N; ++n) sum += pow((double)n, -s);
    sum += pow((double)N, 1.0 - s) / (s - 1.0);
    sum -= 0.5 * pow((double)N, -s);
    return sum;
}

static status_code gamma_series(double eps, double *res) {
    double sum = 0.0;
    int k = 2;
    double term = 1.0;
    while (fabs(term) >= eps && k < 200) {
        double z = zeta((double)k);
        term = (k % 2 == 0 ? 1.0 : -1.0) * z / (double)k;
        sum += term;
        k++;
    }
    *res = sum;
    return SUCCESS;
}

static status_code gamma_equation(double eps, double *res) {
    int limit = (int)(1.0 / eps);
    if (limit < 1000) limit = 1000;
    if (limit > 1000000) limit = 1000000;
    bool *is_prime = (bool *)malloc(((size_t)limit + 1) * sizeof(bool));
    if (!is_prime) return ERROR_MEMORY_ALLOCATION;
    for (int i = 0; i <= limit; ++i) is_prime[i] = true;
    is_prime[0] = is_prime[1] = false;
    for (int p = 2; p * p <= limit; p++) {
        if (is_prime[p]) {
            for (int i = p * p; i <= limit; i += p) is_prime[i] = false;
        }
    }
    double prod = 1.0;
    for (int p = 2; p <= limit; p++) {
        if (is_prime[p]) {
            prod *= ((double)p - 1.0) / (double)p;
        }
    }
    free(is_prime);
    double val = log((double)limit) * prod;
    *res = -log(val);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <epsilon>\n", argv[0]);
        return ERROR_INVALID_ARG_COUNT;
    }
    double eps = 0.0;
    status_code status = parse_double(argv[1], &eps);
    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка: невалидное значение epsilon.\n");
        return status;
    }
    printf("Точность вычислений eps = %g\n\n", eps);
    double res_lim = 0.0, res_ser = 0.0, res_eq = 0.0;
    e_limit(eps, &res_lim); e_series(eps, &res_ser); e_equation(eps, &res_eq);
    printf("Константа e:\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);
    pi_limit(eps, &res_lim); pi_series(eps, &res_ser); pi_equation(eps, &res_eq);
    printf("Константа pi:\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);
    ln2_limit(eps, &res_lim); ln2_series(eps, &res_ser); ln2_equation(eps, &res_eq);
    printf("Константа ln(2):\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);
    sqrt2_limit(eps, &res_lim); sqrt2_series(eps, &res_ser); sqrt2_equation(eps, &res_eq);
    printf("Константа sqrt(2):\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);
    gamma_limit(eps, &res_lim); gamma_series(eps, &res_ser); gamma_equation(eps, &res_eq);
    printf("Константа gamma:\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n", res_lim, res_ser, res_eq);
    return SUCCESS;
}