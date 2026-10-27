#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_PARAM,
    ERROR_FILE_OPEN,
    ERROR_MEMORY_ALLOCATION
} status_code;

static size_t my_strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

static int my_match_at(const char *text, const char *sub, size_t text_len, size_t sub_len, size_t index) {
    if (index + sub_len > text_len) {
        return 0;
    }
    for (size_t i = 0; i < sub_len; ++i) {
        if (text[index + i] != sub[i]) {
            return 0;
        }
    }
    return 1;
}

static status_code read_file_to_buffer(const char *filename, char **buffer, size_t *size) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return ERROR_FILE_OPEN;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return ERROR_FILE_OPEN;
    }

    long length = ftell(file);
    if (length < 0) {
        fclose(file);
        return ERROR_FILE_OPEN;
    }

    rewind(file);

    char *buf = (char *)malloc((size_t)length + 1);
    if (buf == NULL) {
        fclose(file);
        return ERROR_MEMORY_ALLOCATION;
    }

    size_t read_bytes = fread(buf, 1, (size_t)length, file);
    buf[read_bytes] = '\0';
    fclose(file);

    *buffer = buf;
    *size = read_bytes;
    return SUCCESS;
}

static status_code search_in_files_variadic(const char *substring, size_t file_count, ...) {
    if (substring == NULL) {
        return ERROR_INVALID_PARAM;
    }

    size_t sub_len = my_strlen(substring);
    if (sub_len == 0) {
        return ERROR_INVALID_PARAM;
    }

    va_list args;
    va_start(args, file_count);

    for (size_t f = 0; f < file_count; ++f) {
        const char *filename = va_arg(args, const char *);
        if (filename == NULL) {
            va_end(args);
            return ERROR_INVALID_PARAM;
        }

        char *content = NULL;
        size_t content_len = 0;
        status_code status = read_file_to_buffer(filename, &content, &content_len);
        if (status != SUCCESS) {
            va_end(args);
            return status;
        }

        printf("Файл: %s\n", filename);

        for (size_t i = 0; i <= content_len; ++i) {
            if (my_match_at(content, substring, content_len, sub_len, i)) {
                size_t line = 1;
                size_t col = 1;
                for (size_t k = 0; k < i; ++k) {
                    if (content[k] == '\n') {
                        line++;
                        col = 1;
                    } else {
                        col++;
                    }
                }
                printf("  Найдено: строка %zu, символ %zu\n", line, col);
            }
        }

        free(content);
    }

    va_end(args);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Ошибка: недостаточно аргументов.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *substring = argv[1];
    size_t file_count = (size_t)(argc - 2);

    status_code status = SUCCESS;

    if (file_count == 1) {
        status = search_in_files_variadic(substring, 1, argv[2]);
    } else if (file_count == 2) {
        status = search_in_files_variadic(substring, 2, argv[2], argv[3]);
    } else if (file_count == 3) {
        status = search_in_files_variadic(substring, 3, argv[2], argv[3], argv[4]);
    } else {
        fprintf(stderr, "Ошибка: поддерживается до 3 файлов в текущей демонстрации.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка при обработке файлов.\n");
        return status;
    }

    return SUCCESS;
}