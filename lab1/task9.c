#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_INPUT,
    ERROR_MEMORY_ALLOCATION
} status_code;

#define FIXED_SIZE 15

static status_code parse_int(const char *str, int *result) {
    if (str == NULL || result == NULL || *str == '\0') {
        return ERROR_INVALID_INPUT;
    }
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);

    if (endptr == str || *endptr != '\0' || val > INT_MAX || val < INT_MIN) {
        return ERROR_INVALID_INPUT;
    }
    *result = (int)val;
    return SUCCESS;
}

static int random_in_range(int min, int max) {
    long long range = (long long)max - (long long)min + 1;
    if (range <= 0) return min;
    return min + (int)(random() % range);
}

static status_code process_part1(int a, int b, int *result_arr) {
    if (a > b) return ERROR_INVALID_INPUT;

    for (int i = 0; i < FIXED_SIZE; i++) {
        result_arr[i] = random_in_range(a, b);
    }

    size_t min_idx = 0;
    size_t max_idx = 0;
    for (size_t i = 1; i < FIXED_SIZE; i++) {
        if (result_arr[i] < result_arr[min_idx]) {
            min_idx = i;
        }
        if (result_arr[i] > result_arr[max_idx]) {
            max_idx = i;
        }
    }

    int temp = result_arr[min_idx];
    result_arr[min_idx] = result_arr[max_idx];
    result_arr[max_idx] = temp;

    return SUCCESS;
}

static void print_array(const char *label, const int *arr, size_t size) {
    printf("%s: ", label);
    for (size_t i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

static status_code process_part2(int **c_out, size_t *c_size_out) {
    size_t size_a = (size_t)(random() % (10000 - 10 + 1) + 10);
    size_t size_b = (size_t)(random() % (10000 - 10 + 1) + 10);

    int *a = (int *)malloc(size_a * sizeof(int));
    if (a == NULL) return ERROR_MEMORY_ALLOCATION;

    int *b = (int *)malloc(size_b * sizeof(int));
    if (b == NULL) {
        free(a);
        return ERROR_MEMORY_ALLOCATION;
    }

    int *c = (int *)malloc(size_a * sizeof(int));
    if (c == NULL) {
        free(a);
        free(b);
        return ERROR_MEMORY_ALLOCATION;
    }

    for (size_t i = 0; i < size_a; i++) {
        a[i] = random_in_range(-1000, 1000);
    }

    for (size_t i = 0; i < size_b; i++) {
        b[i] = random_in_range(-1000, 1000);
    }

    for (size_t i = 0; i < size_a; i++) {
        int closest = b[0];
        int min_diff = abs(a[i] - b[0]);

        for (size_t j = 1; j < size_b; j++) {
            int diff = abs(a[i] - b[j]);
            if (diff < min_diff) {
                min_diff = diff;
                closest = b[j];
            }
        }
        c[i] = a[i] + closest;
    }

    free(a);
    free(b);

    *c_out = c;
    *c_size_out = size_a;
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <a> <b>\n", argv[0]);
        return ERROR_INVALID_ARG_COUNT;
    }

    int a = 0;
    int b = 0;
    if (parse_int(argv[1], &a) != SUCCESS || parse_int(argv[2], &b) != SUCCESS) {
        fprintf(stderr, "Ошибка: неверный формат диапазона a и b.\n");
        return ERROR_INVALID_INPUT;
    }
    if (a > b) {
        fprintf(stderr, "Ошибка: значение a должно быть меньше или равно b.\n");
        return ERROR_INVALID_INPUT;
    }

    srandom((unsigned int)time(NULL));

    int arr1[FIXED_SIZE];
    if (process_part1(a, b, arr1) != SUCCESS) {
        fprintf(stderr, "Ошибка в части 1.\n");
        return ERROR_INVALID_INPUT;
    }
    printf("Часть 1.\n");
    print_array("Исходный массив", arr1, FIXED_SIZE);
    printf("\n");

    int *c = NULL;
    size_t c_size = 0;
    if (process_part2(&c, &c_size) != SUCCESS) {
        fprintf(stderr, "Ошибка при выполнении части 2.\n");
        return ERROR_MEMORY_ALLOCATION;
    }

    printf("Часть 2 успешно выполнена.\n");
    printf("Размер массива C: %zu\n", c_size);
    printf("Первые 5 элементов массива C: ");
    for (size_t i = 0; i < 5 && i < c_size; i++) {
        printf("%d ", c[i]);
    }
    printf("\n");

    free(c);
    return SUCCESS;
}