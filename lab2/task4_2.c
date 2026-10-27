#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[64];
    unsigned long long size;
    unsigned long long importance;
} FileInfo;

typedef struct {
    FileInfo *files;
    size_t count;
    unsigned long long total_size;
    unsigned long long total_importance;
} BackupResult;

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_PARAM,
    ERROR_MEMORY_ALLOCATION,
    ERROR_FILE_FORMAT
} status_code;

static status_code select_backup_files(const FileInfo *files, size_t n, unsigned long long capacity, BackupResult *result) {
    if (files == NULL || result == NULL) {
        return ERROR_INVALID_PARAM;
    }

    if (n == 0 || capacity == 0) {
        result->files = NULL;
        result->count = 0;
        result->total_size = 0;
        result->total_importance = 0;
        return SUCCESS;
    }

    unsigned long long *dp = (unsigned long long *)calloc((size_t)(capacity + 1), sizeof(unsigned long long));
    if (dp == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    unsigned char **keep = (unsigned char **)malloc(n * sizeof(unsigned char *));
    if (keep == NULL) {
        free(dp);
        return ERROR_MEMORY_ALLOCATION;
    }

    for (size_t i = 0; i < n; ++i) {
        keep[i] = (unsigned char *)calloc((size_t)(capacity + 1), sizeof(unsigned char));
        if (keep[i] == NULL) {
            for (size_t j = 0; j < i; ++j) {
                free(keep[j]);
            }
            free(keep);
            free(dp);
            return ERROR_MEMORY_ALLOCATION;
        }
    }

    for (size_t i = 0; i < n; ++i) {
        unsigned long long w = files[i].size;
        unsigned long long v = files[i].importance;
        for (unsigned long long w_curr = capacity; w_curr >= w; --w_curr) {
            if (w_curr - w <= capacity) {
                unsigned long long without_item = dp[w_curr];
                unsigned long long with_item = dp[w_curr - w] + v;
                if (with_item > without_item) {
                    dp[w_curr] = with_item;
                    keep[i][w_curr] = 1;
                }
            }
            if (w_curr == 0) {
                break;
            }
        }
    }

    unsigned long long best_w = 0;
    unsigned long long max_val = 0;
    for (unsigned long long w_curr = 0; w_curr <= capacity; ++w_curr) {
        if (dp[w_curr] > max_val) {
            max_val = dp[w_curr];
            best_w = w_curr;
        }
    }

    size_t count = 0;
    unsigned long long curr_w = best_w;
    for (size_t i = n; i > 0; --i) {
        size_t idx = i - 1;
        if (keep[idx][curr_w]) {
            count++;
            curr_w -= files[idx].size;
        }
    }

    FileInfo *selected = NULL;
    if (count > 0) {
        selected = (FileInfo *)malloc(count * sizeof(FileInfo));
        if (selected == NULL) {
            for (size_t i = 0; i < n; ++i) {
                free(keep[i]);
            }
            free(keep);
            free(dp);
            return ERROR_MEMORY_ALLOCATION;
        }

        curr_w = best_w;
        size_t index = count;
        for (size_t i = n; i > 0; --i) {
            size_t idx = i - 1;
            if (keep[idx][curr_w]) {
                selected[--index] = files[idx];
                curr_w -= files[idx].size;
            }
        }
    }

    result->files = selected;
    result->count = count;
    result->total_size = best_w;
    result->total_importance = max_val;

    for (size_t i = 0; i < n; ++i) {
        free(keep[i]);
    }
    free(keep);
    free(dp);

    return SUCCESS;
}

int main(void) {
    char n_str[64];
    char cap_str[64];

    if (scanf("%63s %63s", n_str, cap_str) != 2) {
        fprintf(stderr, "Ошибка чтения n и capacity.\n");
        return ERROR_FILE_FORMAT;
    }

    char *endptr = NULL;
    unsigned long long n = strtoull(n_str, &endptr, 10);
    if (*endptr != '\0') {
        return ERROR_FILE_FORMAT;
    }
    unsigned long long capacity = strtoull(cap_str, &endptr, 10);
    if (*endptr != '\0') {
        return ERROR_FILE_FORMAT;
    }

    if (n == 0) {
        printf("TOTAL_SIZE 0\n");
        printf("TOTAL_IMPORTANCE 0\n");
        printf("FILES 0\n");
        return SUCCESS;
    }

    FileInfo *files = (FileInfo *)malloc((size_t)n * sizeof(FileInfo));
    if (files == NULL) {
        fprintf(stderr, "Ошибка выделения памяти.\n");
        return ERROR_MEMORY_ALLOCATION;
    }

    for (unsigned long long i = 0; i < n; ++i) {
        char size_str[64];
        char imp_str[64];
        if (scanf("%63s %63s %63s", files[i].name, size_str, imp_str) != 3) {
            fprintf(stderr, "Ошибка чтения данных файла.\n");
            free(files);
            return ERROR_FILE_FORMAT;
        }

        files[i].size = strtoull(size_str, &endptr, 10);
        if (*endptr != '\0') {
            free(files);
            return ERROR_FILE_FORMAT;
        }
        files[i].importance = strtoull(imp_str, &endptr, 10);
        if (*endptr != '\0') {
            free(files);
            return ERROR_FILE_FORMAT;
        }
    }

    BackupResult result;
    status_code status = select_backup_files(files, (size_t)n, capacity, &result);
    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка при выполнении алгоритма выбора файлов.\n");
        free(files);
        return status;
    }

    printf("TOTAL_SIZE %llu\n", result.total_size);
    printf("TOTAL_IMPORTANCE %llu\n", result.total_importance);
    printf("FILES %zu\n", result.count);

    for (size_t i = 0; i < result.count; ++i) {
        printf("%s\n", result.files[i].name);
    }

    free(result.files);
    free(files);

    return SUCCESS;
}