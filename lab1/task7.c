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

static status_code read_lexeme(FILE *f, char **lexeme_out) {
    if (f == NULL || lexeme_out == NULL) {
        return ERROR_INVALID_FLAG;
    }

    size_t cap = 64;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (buf == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    int ch = 0;
    while ((ch = fgetc(f)) != EOF && isspace(ch)) {
    }

    if (ch == EOF) {
        free(buf);
        *lexeme_out = NULL;
        return SUCCESS;
    }

    do {
        if (len + 1 >= cap) {
            cap *= 2;
            char *new_buf = (char *)realloc(buf, cap);
            if (new_buf == NULL) {
                free(buf);
                return ERROR_MEMORY_ALLOCATION;
            }
            buf = new_buf;
        }
        buf[len++] = (char)ch;
        ch = fgetc(f);
    } while (ch != EOF && !isspace(ch));

    buf[len] = '\0';
    *lexeme_out = buf;
    return SUCCESS;
}

static void to_base_string(unsigned int val, int base, char *out_buf, size_t out_size) {
    if (val == 0) {
        snprintf(out_buf, out_size, "0");
        return;
    }
    char temp[64];
    int idx = 0;
    while (val > 0 && idx < (int)sizeof(temp) - 1) {
        temp[idx++] = (char)('0' + (val % base));
        val /= base;
    }
    int dest = 0;
    for (int i = idx - 1; i >= 0 && dest < (int)out_size - 1; i--) {
        out_buf[dest++] = temp[i];
    }
    out_buf[dest] = '\0';
}

static status_code convert_lexeme_to_base(const char *lexeme, int base, char **result_out) {
    size_t total_len = 0;
    for (size_t i = 0; lexeme[i] != '\0'; i++) {
        unsigned char val = (unsigned char)lexeme[i];
        char conv[32];
        to_base_string(val, base, conv, sizeof(conv));
        total_len += strlen(conv);
    }

    char *res = (char *)malloc(total_len + 1);
    if (res == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    res[0] = '\0';

    for (size_t i = 0; lexeme[i] != '\0'; i++) {
        unsigned char val = (unsigned char)lexeme[i];
        char conv[32];
        to_base_string(val, base, conv, sizeof(conv));
        strcat(res, conv);
    }

    *result_out = res;
    return SUCCESS;
}

static char *to_lower_lexeme(const char *lexeme) {
    size_t len = strlen(lexeme);
    char *res = (char *)malloc(len + 1);
    if (res == NULL) {
        return NULL;
    }
    for (size_t i = 0; i <= len; i++) {
        res[i] = (char)tolower((unsigned char)lexeme[i]);
    }
    return res;
}

static char *clone_string(const char *str) {
    size_t len = strlen(str);
    char *res = (char *)malloc(len + 1);
    if (res == NULL) {
        return NULL;
    }
    strcpy(res, str);
    return res;
}

static status_code process_r(const char *file1, const char *file2, const char *out_file) {
    FILE *f1 = fopen(file1, "r");
    if (f1 == NULL) return ERROR_FILE_OPEN;

    FILE *f2 = fopen(file2, "r");
    if (f2 == NULL) {
        fclose(f1);
        return ERROR_FILE_OPEN;
    }

    FILE *fout = fopen(out_file, "w");
    if (fout == NULL) {
        fclose(f1);
        fclose(f2);
        return ERROR_FILE_OPEN;
    }

    char *lex1 = NULL;
    char *lex2 = NULL;
    bool first_written = false;

    while (true) {
        status_code st1 = read_lexeme(f1, &lex1);
        status_code st2 = read_lexeme(f2, &lex2);

        if (st1 != SUCCESS || st2 != SUCCESS) {
            free(lex1);
            free(lex2);
            fclose(f1);
            fclose(f2);
            fclose(fout);
            return ERROR_MEMORY_ALLOCATION;
        }

        if (lex1 == NULL && lex2 == NULL) {
            break;
        }

        if (lex1 != NULL) {
            if (first_written) fputc(' ', fout);
            fputs(lex1, fout);
            first_written = true;
            free(lex1);
            lex1 = NULL;
        }

        if (lex2 != NULL) {
            if (first_written) fputc(' ', fout);
            fputs(lex2, fout);
            first_written = true;
            free(lex2);
            lex2 = NULL;
        }
    }

    fclose(f1);
    fclose(f2);
    fclose(fout);
    return SUCCESS;
}

static status_code process_a(const char *in_file, const char *out_file) {
    FILE *fin = fopen(in_file, "r");
    if (fin == NULL) return ERROR_FILE_OPEN;

    FILE *fout = fopen(out_file, "w");
    if (fout == NULL) {
        fclose(fin);
        return ERROR_FILE_OPEN;
    }

    char *lexeme = NULL;
    unsigned long long k = 0;
    bool first_written = false;

    while (true) {
        status_code st = read_lexeme(fin, &lexeme);
        if (st != SUCCESS) {
            fclose(fin);
            fclose(fout);
            return ERROR_MEMORY_ALLOCATION;
        }

        if (lexeme == NULL) {
            break;
        }

        k++;
        char *processed = NULL;

        if (k % 10 == 0) {
            char *lowered = to_lower_lexeme(lexeme);
            if (lowered == NULL) {
                free(lexeme);
                fclose(fin);
                fclose(fout);
                return ERROR_MEMORY_ALLOCATION;
            }
            status_code cst = convert_lexeme_to_base(lowered, 4, &processed);
            free(lowered);
            if (cst != SUCCESS) {
                free(lexeme);
                fclose(fin);
                fclose(fout);
                return cst;
            }
        } else if (k % 2 == 0) {
            processed = to_lower_lexeme(lexeme);
            if (processed == NULL) {
                free(lexeme);
                fclose(fin);
                fclose(fout);
                return ERROR_MEMORY_ALLOCATION;
            }
        } else if (k % 5 == 0) {
            status_code cst = convert_lexeme_to_base(lexeme, 8, &processed);
            if (cst != SUCCESS) {
                free(lexeme);
                fclose(fin);
                fclose(fout);
                return cst;
            }
        } else {
            processed = clone_string(lexeme);
            if (processed == NULL) {
                free(lexeme);
                fclose(fin);
                fclose(fout);
                return ERROR_MEMORY_ALLOCATION;
            }
        }

        if (first_written) {
            fputc(' ', fout);
        }
        fputs(processed, fout);
        first_written = true;

        free(lexeme);
        free(processed);
        lexeme = NULL;
    }

    fclose(fin);
    fclose(fout);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка: недостаточно аргументов.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *flag = argv[1];
    if ((flag[0] != '-' && flag[0] != '/') || flag[1] == '\0' || flag[2] != '\0') {
        fprintf(stderr, "Ошибка: неверный формат флага.\n");
        return ERROR_INVALID_FLAG;
    }

    char flag_char = flag[1];

    if (flag_char == 'r') {
        if (argc != 5) {
            fprintf(stderr, "Ошибка: флаг -r требует 3 аргумента: <file1> <file2> <out_file>\n");
            return ERROR_INVALID_ARG_COUNT;
        }
        return process_r(argv[2], argv[3], argv[4]);
    } else if (flag_char == 'a') {
        if (argc != 4) {
            fprintf(stderr, "Ошибка: флаг -a требует 2 аргумента: <in_file> <out_file>\n");
            return ERROR_INVALID_ARG_COUNT;
        }
        return process_a(argv[2], argv[3]);
    } else {
        fprintf(stderr, "Ошибка: неизвестный флаг '%s'.\n", flag);
        return ERROR_INVALID_FLAG;
    }
}