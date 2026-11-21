#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

typedef enum status_code {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_FLAG,
    ERROR_FILE_OPEN,
    ERROR_MEMORY_ALLOCATION,
    ERROR_SAME_FILE,
    ERROR_OVERFLOW
} status_code;

static status_code read_lexeme(FILE *f, char **lexeme_out) {
    if (f == NULL || lexeme_out == NULL) return ERROR_INVALID_FLAG;

    size_t cap = 64;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (buf == NULL) return ERROR_MEMORY_ALLOCATION;

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

static int get_digit_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static status_code find_min_base(const char *lexeme, int *min_base_out) {
    int max_digit = 0;
    bool has_valid_chars = false;

    for (size_t i = 0; lexeme[i] != '\0'; i++) {
        int val = get_digit_value(lexeme[i]);
        if (val < 0) return ERROR_INVALID_FLAG;
        if (val > max_digit) max_digit = val;
        has_valid_chars = true;
    }

    if (!has_valid_chars) return ERROR_INVALID_FLAG;

    int base = max_digit + 1;
    if (base < 2) base = 2;
    if (base > 36) return ERROR_INVALID_FLAG;

    *min_base_out = base;
    return SUCCESS;
}

static void remove_leading_zeros(const char *lexeme, const char **stripped_out) {
    size_t i = 0;
    while (lexeme[i] == '0' && lexeme[i + 1] != '\0') {
        i++;
    }
    *stripped_out = lexeme + i;
}

static status_code parse_to_decimal(const char *lexeme, int base, unsigned long long *val_out) {
    unsigned long long result = 0;

    for (size_t i = 0; lexeme[i] != '\0'; i++) {
        int digit = get_digit_value(lexeme[i]);
        if (digit < 0 || digit >= base) return ERROR_INVALID_FLAG;

        unsigned long long max_val = ~0ULL;
        if (result > (max_val - (unsigned long long)digit) / (unsigned long long)base) {
            return ERROR_OVERFLOW;
        }
        result = result * (unsigned long long)base + (unsigned long long)digit;
    }
    *val_out = result;
    return SUCCESS;
}

static bool is_same_file(const char *path1, const char *path2) {
    struct stat st1, st2;
    if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) return false;
    return st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino;
}

static status_code process_file(const char *in_file, const char *out_file) {
    if (is_same_file(in_file, out_file)) {
        fprintf(stderr, "Ошибка: входной и выходной файлы совпадают!\n");
        return ERROR_SAME_FILE;
    }

    FILE *fin = fopen(in_file, "r");
    if (fin == NULL) return ERROR_FILE_OPEN;

    FILE *fout = fopen(out_file, "w");
    if (fout == NULL) {
        fclose(fin);
        return ERROR_FILE_OPEN;
    }

    char *lexeme = NULL;
    status_code st = SUCCESS;

    while (true) {
        st = read_lexeme(fin, &lexeme);
        if (st != SUCCESS) {
            fclose(fin);
            fclose(fout);
            return st;
        }

        if (lexeme == NULL) break;

        int base = 0;
        if (find_min_base(lexeme, &base) != SUCCESS) {
            free(lexeme);
            lexeme = NULL;
            continue;
        }

        const char *stripped = NULL;
        remove_leading_zeros(lexeme, &stripped);

        unsigned long long dec_val = 0;
        if (parse_to_decimal(stripped, base, &dec_val) != SUCCESS) {
            free(lexeme);
            lexeme = NULL;
            continue;
        }

        fprintf(fout, "%s %d %llu\n", stripped, base, dec_val);
        free(lexeme);
        lexeme = NULL;
    }

    fclose(fin);
    fclose(fout);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <input_file> <output_file>\n", argv[0]);
        return ERROR_INVALID_ARG_COUNT;
    }

    return process_file(argv[1], argv[2]);
}