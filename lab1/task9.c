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

static status_code process_part1(int a, int b) {
    if (a > b) {
        return ERROR_INVALID_INPUT;
    }

    int arr[FIXED_SIZE];
    for (int i = 0; i < FIXED_SIZE; i++) {
        arr[i] = a + rand() % (b - a + 1);
    }

    printf("Часть 1. Исходный массив:\n");
    for (int i = 0; i < FIXED_SIZE; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    size_t min_idx = 0;
    size_t max_idx = 0;
    for (size_t i = 1; i < FIXED_SIZE; i++) {
        if (arr[i] < arr[min_idx]) {
            min_idx = i;
        }
        if (arr[i] > arr[max_idx]) {
            max_idx = i;
        }
    }

    int temp = arr[min_idx];
    arr[min_idx] = arr[max_idx];
    arr[max_idx] = temp;

    printf("Массив после обмена минимального и максимального элементов:\n");
    for (int i = 0; i < FIXED_SIZE; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n\n");

    return SUCCESS;
}

static status_code process_part2(void) {
    size_t size_a = (size_t)(rand() % (10000 - 10 + 1) + 10);
    size_t size_b = (size_t)(rand() % (10000 - 10 + 1) + 10);

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
        a[i] = (rand() % 2001) - 1000;
    }

    for (size_t i = 0; i < size_b; i++) {
        b[i] = (rand() % 2001) - 1000;
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

    printf("Часть 2 успешно выполнена.\n");
    printf("Размер массива A: %zu, размер массива B: %zu\n", size_a, size_b);
    printf("Первые 5 элементов массива A: ");
    for (size_t i = 0; i < 5 && i < size_a; i++) {
        printf("%d ", a[i]);
    }
    printf("\n");
    printf("Первые 5 элементов массива C: ");
    for (size_t i = 0; i < 5 && i < size_a; i++) {
        printf("%d ", c[i]);
    }
    printf("\n");

    free(a);
    free(b);
    free(c);

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

    srand((unsigned int)time(NULL));

    status_code st = process_part1(a, b);
    if (st != SUCCESS) {
        return st;
    }

    st = process_part2();
    if (st != SUCCESS) {
        return st;
    }

    return SUCCESS;
}