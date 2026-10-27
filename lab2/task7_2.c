#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef struct {
    double *coords;
} Vector;

typedef double (*NormFunc)(int n, const Vector *v, ...);

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION
} status_code;

static double norm_infinity(int n, const Vector *v, ...) {
    double max_val = 0.0;
    for (int i = 0; i < n; ++i) {
        double val = fabs(v->coords[i]);
        if (val > max_val) {
            max_val = val;
        }
    }
    return max_val;
}

static double norm_p(int n, const Vector *v, ...) {
    va_list args;
    va_start(args, v);
    double p = va_arg(args, double);
    va_end(args);

    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += pow(fabs(v->coords[i]), p);
    }
    return pow(sum, 1.0 / p);
}

static double norm_matrix(int n, const Vector *v, ...) {
    va_list args;
    va_start(args, v);
    const double *matrix_data = va_arg(args, const double *);
    va_end(args);

    double *ax = (double *)malloc(n * sizeof(double));
    if (ax == NULL) {
        return -1.0;
    }

    for (int i = 0; i < n; ++i) {
        ax[i] = 0.0;
        for (int j = 0; j < n; ++j) {
            ax[i] += matrix_data[i * n + j] * v->coords[j];
        }
    }

    double scalar_prod = 0.0;
    for (int i = 0; i < n; ++i) {
        scalar_prod += ax[i] * v->coords[i];
    }

    free(ax);
    return sqrt(scalar_prod);
}

static status_code find_longest_vectors(int n, int vector_count, int norm_count, const Vector *vectors, const NormFunc *norms, int ***results, int **result_counts) {
    if (n <= 0 || vector_count <= 0 || norm_count <= 0 || vectors == NULL || norms == NULL || results == NULL || result_counts == NULL) {
        return ERROR_INVALID_PARAM;
    }

    int *counts = (int *)malloc(norm_count * sizeof(int));
    int **res_indices = (int **)malloc(norm_count * sizeof(int *));
    if (counts == NULL || res_indices == NULL) {
        free(counts);
        free(res_indices);
        return ERROR_MEMORY_ALLOCATION;
    }

    for (int i = 0; i < norm_count; ++i) {
        double max_len = -1.0;
        int current_count = 0;
        int *temp_indices = (int *)malloc(vector_count * sizeof(int));
        if (temp_indices == NULL) {
            for (int j = 0; j < i; ++j) {
                free(res_indices[j]);
            }
            free(res_indices);
            free(counts);
            return ERROR_MEMORY_ALLOCATION;
        }

        for (int j = 0; j < vector_count; ++j) {
            double length = norms[i](n, &vectors[j]);
            if (length > max_len + 1e-9) {
                max_len = length;
                current_count = 0;
                temp_indices[current_count++] = j;
            } else if (fabs(length - max_len) <= 1e-9) {
                temp_indices[current_count++] = j;
            }
        }

        counts[i] = current_count;
        res_indices[i] = (int *)malloc(current_count * sizeof(int));
        if (res_indices[i] == NULL) {
            free(temp_indices);
            for (int j = 0; j < i; ++j) {
                free(res_indices[j]);
            }
            free(res_indices);
            free(counts);
            return ERROR_MEMORY_ALLOCATION;
        }

        for (int j = 0; j < current_count; ++j) {
            res_indices[i][j] = temp_indices[j];
        }
        free(temp_indices);
    }

    *results = res_indices;
    *result_counts = counts;
    return SUCCESS;
}

int main(void) {
    int n = 2;
    int vector_count = 3;
    int norm_count = 3;

    Vector vectors[3];
    vectors[0].coords = (double *)malloc(2 * sizeof(double));
    vectors[1].coords = (double *)malloc(2 * sizeof(double));
    vectors[2].coords = (double *)malloc(2 * sizeof(double));

    vectors[0].coords[0] = 3.0; vectors[0].coords[1] = 4.0;
    vectors[1].coords[0] = -5.0; vectors[1].coords[1] = 0.0;
    vectors[2].coords[0] = 3.0; vectors[2].coords[1] = 4.0;

    NormFunc norms[3];
    norms[0] = norm_infinity;
    norms[1] = norm_p;
    norms[2] = norm_matrix;

    int **results = NULL;
    int *counts = NULL;

    status_code status = find_longest_vectors(n, vector_count, norm_count, vectors, norms, &results, &counts);
    if (status != SUCCESS) {
        for (int i = 0; i < vector_count; ++i) {
            free(vectors[i].coords);
        }
        return status;
    }

    for (int i = 0; i < norm_count; ++i) {
        printf("Norm %d longest vectors count: %d\n", i + 1, counts[i]);
        for (int j = 0; j < counts[i]; ++j) {
            int idx = results[i][j];
            printf("Vector %d: (%.1f, %.1f)\n", idx, vectors[idx].coords[0], vectors[idx].coords[1]);
        }
        free(results[i]);
    }

    free(results);
    free(counts);

    for (int i = 0; i < vector_count; ++i) {
        free(vectors[i].coords);
    }

    return SUCCESS;
}