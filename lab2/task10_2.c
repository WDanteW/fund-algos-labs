#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION
} status_code;

static status_code expand_polynomial(double eps, double a, double **result_coeffs, int degree, ...) {
    if (eps <= 0.0 || degree < 0 || result_coeffs == NULL) {
        return ERROR_INVALID_PARAM;
    }

    int n = degree;
    double *f = (double *)malloc((size_t)(n + 1) * sizeof(double));
    double *g = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (f == NULL || g == NULL) {
        free(f);
        free(g);
        return ERROR_MEMORY_ALLOCATION;
    }

    va_list args;
    va_start(args, degree);
    for (int i = 0; i <= n; ++i) {
        f[i] = va_arg(args, double);
    }
    va_end(args);

    double *current_f = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (current_f == NULL) {
        free(f);
        free(g);
        return ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i <= n; ++i) {
        current_f[i] = f[i];
    }

    for (int k = 0; k <= n; ++k) {
        double remainder = current_f[n];
        for (int i = n - 1; i >= k; --i) {
            double temp = current_f[i];
            current_f[i] = remainder;
            remainder = temp + remainder * a;
        }

        if (fabs(remainder) < eps) {
            g[k] = 0.0;
        } else {
            g[k] = remainder;
        }
    }

    free(f);
    free(current_f);

    *result_coeffs = g;
    return SUCCESS;
}

int main(void) {
    double eps = 1e-6;
    double a = 8;
    int degree = 8;
    double *g_coeffs = NULL;

    status_code status = expand_polynomial(eps, a, &g_coeffs, degree, 1.0, 2.0, 10);
    if (status == SUCCESS) {
        printf("Polynomial expansion coefficients around (x - %.1f):\n", a);
        for (int i = 0; i <= degree; ++i) {
            printf("g_%d = %.6f\n", i, g_coeffs[i]);
        }
        free(g_coeffs);
    }

    return SUCCESS;
}