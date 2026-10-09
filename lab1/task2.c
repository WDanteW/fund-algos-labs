#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <errno.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_EPSILON,
    ERROR_MEMORY_ALLOCATION
} status_code;

static status_code parse_double(const char *str, double *result) {
    if (str == NULL || result == NULL || *str == '\0') {
        return ERROR_INVALID_EPSILON;
    }

    char *endptr = NULL;
    errno = 0;
    double val = strtod(str, &endptr);

    if (errno == ERANGE || *endptr != '\0') {
        return ERROR_INVALID_EPSILON;
    }

    *result = val;
    return SUCCESS;
}

static status_code e_limit(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double prev = 0.0;
    long long n = 1;
    double current = pow(1.0 + 1.0 / (double)n, (double)n);

    while (fabs(current - prev) >= eps) {
        prev = current;
        n *= 2;
        current = pow(1.0 + 1.0 / (double)n, (double)n);
    }

    *res = current;
    return SUCCESS;
}

static status_code e_series(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double sum = 0.0;
    double term = 1.0;
    int n = 0;

    while (term >= eps) {
        sum += term;
        n++;
        term /= n;
    }

    *res = sum;
    return SUCCESS;
}

static status_code e_equation(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

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
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double prev = 0.0;
    int n = 1;
    double current = 4.0;

    while (n < 100000) {
        double num = pow(pow(2.0, n) * tgamma(n + 1), 4);
        double den = (double)n * pow(tgamma(2 * n + 1), 2);
        current = num / den;
        if (fabs(current - prev) < eps) break;
        prev = current;
        n++;
    }

    *res = current;
    return SUCCESS;
}

static status_code pi_series(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double sum = 0.0;
    double term = 1.0;
    int n = 1;

    while (fabs(term) >= eps) {
        term = ((n % 2 == 1) ? 1.0 : -1.0) / (2.0 * n - 1.0);
        sum += term;
        n++;
    }

    *res = 4.0 * sum;
    return SUCCESS;
}

static status_code pi_equation(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

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
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double prev = 0.0;
    long long n = 1;
    double current = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);

    while (fabs(current - prev) >= eps) {
        prev = current;
        n *= 2;
        current = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
    }

    *res = current;
    return SUCCESS;
}

static status_code ln2_series(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double sum = 0.0;
    double term = 1.0;
    int n = 1;

    while (fabs(term) >= eps) {
        term = ((n % 2 == 1) ? 1.0 : -1.0) / (double)n;
        sum += term;
        n++;
    }

    *res = sum;
    return SUCCESS;
}

static status_code ln2_equation(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

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
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double x_prev = -0.5;
    double x_next = x_prev - (x_prev * x_prev) / 2.0 + 1.0;

    while (fabs(x_next - x_prev) >= eps) {
        x_prev = x_next;
        x_next = x_prev - (x_prev * x_prev) / 2.0 + 1.0;
    }

    *res = x_next;
    return SUCCESS;
}

static status_code sqrt2_series(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double prod = 1.0;
    int k = 2;
    double factor = pow(2.0, pow(2.0, -k));

    while (fabs(factor - 1.0) >= eps) {
        prod *= factor;
        k++;
        factor = pow(2.0, pow(2.0, -k));
    }

    *res = prod;
    return SUCCESS;
}

static status_code sqrt2_equation(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

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
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double prev = 0.0;
    int m = 1;
    double current = 0.0;

    while (m < 50) {
        double sum = 0.0;
        double log_fact_k = 0.0;

        double c_m_k = 1.0;
        for (int k = 1; k <= m; ++k) {
            c_m_k = c_m_k * (m - k + 1) / k;
            log_fact_k += log((double)k);

            double term = c_m_k * ((k % 2 == 1) ? -1.0 : 1.0) / k * log_fact_k;
            sum += term;
        }

        current = sum;
        if (fabs(current - prev) < eps) break;
        prev = current;
        m++;
    }

    *res = current;
    return SUCCESS;
}

static status_code gamma_series(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    double pi_val = 0.0;
    pi_equation(1e-12, &pi_val);

    double sum = 0.0;
    int k = 2;
    double term = 1.0;

    while (fabs(term) >= eps) {
        double floor_sqrt_k = floor(sqrt((double)k));
        term = (1.0 / (floor_sqrt_k * floor_sqrt_k)) - (1.0 / (double)k);
        sum += term;
        k++;
    }

    *res = -(pi_val * pi_val) / 6.0 + sum;
    return SUCCESS;
}

static status_code gamma_equation(double eps, double *res) {
    if (res == NULL) return ERROR_INVALID_EPSILON;

    int limit = (int)(1.0 / eps);
    if (limit < 10) limit = 10;
    if (limit > 100000) limit = 100000;

    bool *is_prime = (bool *)malloc(((size_t)limit + 1) * sizeof(bool));
    if (is_prime == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i <= limit; ++i) is_prime[i] = true;
    is_prime[0] = is_prime[1] = false;

    for (int p = 2; p * p <= limit; p++) {
        if (is_prime[p]) {
            for (int i = p * p; i <= limit; i += p) {
                is_prime[i] = false;
            }
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
    if (status != SUCCESS || eps <= 0.0 || eps >= 1.0) {
        fprintf(stderr, "Ошибка: невалидное значение epsilon (должно быть в интервале (0, 1)).\n");
        return ERROR_INVALID_EPSILON;
    }

    printf("Точность вычислений eps = %g\n\n", eps);

    double res_lim = 0.0, res_ser = 0.0, res_eq = 0.0;

    e_limit(eps, &res_lim);
    e_series(eps, &res_ser);
    e_equation(eps, &res_eq);
    printf("Константа e:\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);

    pi_limit(eps, &res_lim);
    pi_series(eps, &res_ser);
    pi_equation(eps, &res_eq);
    printf("Константа pi:\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);

    ln2_limit(eps, &res_lim);
    ln2_series(eps, &res_ser);
    ln2_equation(eps, &res_eq);
    printf("Константа ln(2):\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);

    sqrt2_limit(eps, &res_lim);
    sqrt2_series(eps, &res_ser);
    sqrt2_equation(eps, &res_eq);
    printf("Константа sqrt(2):\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n\n", res_lim, res_ser, res_eq);

    gamma_limit(eps, &res_lim);
    gamma_series(eps, &res_ser);
    status = gamma_equation(eps, &res_eq);
    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка при вычислении gamma через уравнение.\n");
        return status;
    }
    printf("Константа gamma:\n  Предел:    %.10f\n  Ряд:       %.10f\n  Уравнение: %.10f\n", res_lim, res_ser, res_eq);

    return SUCCESS;
}