#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_FLAG,
    ERROR_FILE_OPEN,
    ERROR_MEMORY_ALLOCATION
} status_code;

static status_code create_out_filename(const char *in_path, char **out_path) {
    if (in_path == NULL || out_path == NULL) {
        return ERROR_INVALID_FLAG;
    }

    const char *last_slash = strrchr(in_path, '/');
    size_t dir_len = (last_slash != NULL) ? (size_t)(last_slash - in_path + 1) : 0;
    size_t filename_len = strlen(in_path + dir_len);

    size_t total_len = dir_len + 4 + filename_len + 1;
    char *result = (char *)malloc(total_len);
    if (result == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    if (dir_len > 0) {
        strncpy(result, in_path, dir_len);
        result[dir_len] = '\0';
    } else {
        result[0] = '\0';
    }

    strcat(result, "out_");
    strcat(result, in_path + dir_len);

    *out_path = result;
    return SUCCESS;
}

static status_code process_d(FILE *fin, FILE *fout) {
    int ch = 0;
    while ((ch = fgetc(fin)) != EOF) {
        if (!isdigit(ch)) {
            fputc(ch, fout);
        }
    }
    return SUCCESS;
}

static status_code process_i(FILE *fin, FILE *fout) {
    int ch = 0;
    long count = 0;

    while ((ch = fgetc(fin)) != EOF) {
        if (isalpha(ch)) {
            count++;
        }
        if (ch == '\n') {
            fprintf(fout, "%ld\n", count);
            count = 0;
        }
    }
    if (count > 0 || ftell(fin) > 0) {
        fprintf(fout, "%ld\n", count);
    }
    return SUCCESS;
}

static status_code process_s(FILE *fin, FILE *fout) {
    int ch = 0;
    long count = 0;

    while ((ch = fgetc(fin)) != EOF) {
        if (!isalpha(ch) && !isdigit(ch) && ch != ' ' && ch != '\n' && ch != '\r') {
            count++;
        }
        if (ch == '\n') {
            fprintf(fout, "%ld\n", count);
            count = 0;
        }
    }
    if (count > 0 || ftell(fin) > 0) {
        fprintf(fout, "%ld\n", count);
    }
    return SUCCESS;
}

static status_code process_a(FILE *fin, FILE *fout) {
    int ch = 0;
    while ((ch = fgetc(fin)) != EOF) {
        if (isdigit(ch) || ch == '\n' || ch == '\r') {
            fputc(ch, fout);
        } else {
            fprintf(fout, "%02X", (unsigned char)ch);
        }
    }
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Ошибка: недостаточно аргументов.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *flag_str = argv[1];
    if ((flag_str[0] != '-' && flag_str[0] != '/') || flag_str[1] == '\0') {
        fprintf(stderr, "Ошибка: неверный формат флага.\n");
        return ERROR_INVALID_FLAG;
    }

    bool has_n = (flag_str[1] == 'n');
    char action = has_n ? flag_str[2] : flag_str[1];

    if (has_n && flag_str[3] != '\0') {
        fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", flag_str);
        return ERROR_INVALID_FLAG;
    }
    if (!has_n && flag_str[2] != '\0') {
        fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", flag_str);
        return ERROR_INVALID_FLAG;
    }

    if (has_n && argc != 4) {
        fprintf(stderr, "Ошибка: флаги с 'n' требуют указывать выходной файл.\n");
        return ERROR_INVALID_ARG_COUNT;
    }
    if (!has_n && argc != 3) {
        fprintf(stderr, "Ошибка: данный флаг требует только входной файл.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *in_path = argv[2];
    char *out_path_allocated = NULL;
    const char *out_path = NULL;

    if (has_n) {
        out_path = argv[3];
    } else {
        status_code st = create_out_filename(in_path, &out_path_allocated);
        if (st != SUCCESS) {
            fprintf(stderr, "Ошибка выделения памяти для имени файла.\n");
            return st;
        }
        out_path = out_path_allocated;
    }

    FILE *fin = fopen(in_path, "r");
    if (fin == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть входной файл '%s'.\n", in_path);
        free(out_path_allocated);
        return ERROR_FILE_OPEN;
    }

    FILE *fout = fopen(out_path, "w");
    if (fout == NULL) {
        fprintf(stderr, "Ошибка: не удалось создать выходной файл '%s'.\n", out_path);
        fclose(fin);
        free(out_path_allocated);
        return ERROR_FILE_OPEN;
    }

    status_code status = SUCCESS;

    switch (action) {
        case 'd':
            status = process_d(fin, fout);
            break;
        case 'i':
            status = process_i(fin, fout);
            break;
        case 's':
            status = process_s(fin, fout);
            break;
        case 'a':
            status = process_a(fin, fout);
            break;
        default:
            fprintf(stderr, "Ошибка: неизвестное действие '%c'.\n", action);
            status = ERROR_INVALID_FLAG;
            break;
    }

    fclose(fin);
    fclose(fout);
    free(out_path_allocated);

    return status;
}