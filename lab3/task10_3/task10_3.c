#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_NO_MEMORY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_FILE_WRITE_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_OVERFLOW
} status_t;

typedef struct node {
    char name[64];
    struct node **children;
    int num_children;
    int capacity;
} node_t;

static node_t *node_create(const char *name) {
    if (!name) return NULL;
    node_t *n = (node_t *)malloc(sizeof(node_t));
    if (!n) return NULL;
    strncpy(n->name, name, sizeof(n->name) - 1);
    n->name[sizeof(n->name) - 1] = '\0';
    n->children = NULL;
    n->num_children = 0;
    n->capacity = 0;
    return n;
}

static status_t node_add_child(node_t *parent, node_t *child) {
    if (!parent || !child) return STATUS_NULL_POINTER;
    if (parent->num_children >= parent->capacity) {
        if (parent->capacity > INT_MAX / 2) return STATUS_OVERFLOW;
        int new_cap = parent->capacity == 0 ? 4 : parent->capacity * 2;
        if ((size_t)new_cap > SIZE_MAX / sizeof(node_t *)) return STATUS_OVERFLOW;
        
        node_t **new_ch = (node_t **)realloc(parent->children, new_cap * sizeof(node_t *));
        if (!new_ch) return STATUS_NO_MEMORY;
        parent->children = new_ch;
        parent->capacity = new_cap;
    }
    parent->children[parent->num_children++] = child;
    return STATUS_OK;
}

static void node_free(node_t *node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        node_free(node->children[i]);
    }
    free(node->children);
    free(node);
}

static status_t parse_expression(const char **str_ptr, node_t **out_node) {
    if (!str_ptr || !*str_ptr || !out_node) return STATUS_NULL_POINTER;
    const char *s = *str_ptr;

    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
    if (!*s || *s == ')' || *s == ',') return STATUS_INVALID_INPUT;

    char name_buf[64];
    int n_len = 0;
    while (*s && *s != '(' && *s != ')' && *s != ',' && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') {
        if (n_len >= (int)sizeof(name_buf) - 1) {
            return STATUS_INVALID_INPUT;
        }
        name_buf[n_len++] = *s;
        s++;
    }
    name_buf[n_len] = '\0';
    if (n_len == 0) return STATUS_INVALID_INPUT;

    node_t *node = node_create(name_buf);
    if (!node) return STATUS_NO_MEMORY;

    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;

    if (*s == '(') {
        s++;
        while (1) {
            while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
            if (!*s) {
                node_free(node);
                return STATUS_INVALID_INPUT;
            }
            if (*s == ')') {
                s++;
                break;
            }
            node_t *child = NULL;
            status_t st = parse_expression(&s, &child);
            if (st != STATUS_OK) {
                node_free(node);
                return st;
            }
            st = node_add_child(node, child);
            if (st != STATUS_OK) {
                node_free(child);
                node_free(node);
                return st;
            }
            while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
            if (*s == ',') {
                s++;
            } else if (*s == ')') {
                s++;
                break;
            } else {
                node_free(node);
                return STATUS_INVALID_INPUT;
            }
        }
    }

    *str_ptr = s;
    *out_node = node;
    return STATUS_OK;
}

static status_t print_tree(FILE *f, const node_t *node, int depth) {
    if (!f || !node) return STATUS_NULL_POINTER;
    for (int i = 0; i < depth; i++) {
        if (fprintf(f, "  ") < 0) return STATUS_FILE_WRITE_ERROR;
    }
    if (fprintf(f, "%s\n", node->name) < 0) return STATUS_FILE_WRITE_ERROR;
    for (int i = 0; i < node->num_children; i++) {
        status_t st = print_tree(f, node->children[i], depth + 1);
        if (st != STATUS_OK) return st;
    }
    return STATUS_OK;
}

static char *read_line_dynamic(FILE *f, status_t *st) {
    size_t cap = 256;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) {
        *st = STATUS_NO_MEMORY;
        return NULL;
    }
    int ch;
    while ((ch = fgetc(f)) != EOF && ch != '\n') {
        if (ch == '\r') continue;
        if (len + 1 >= cap) {
            cap *= 2;
            char *new_buf = (char *)realloc(buf, cap);
            if (!new_buf) {
                free(buf);
                *st = STATUS_NO_MEMORY;
                return NULL;
            }
            buf = new_buf;
        }
        buf[len++] = (char)ch;
    }
    if (ch == EOF && len == 0) {
        free(buf);
        *st = STATUS_FILE_READ_ERROR;
        return NULL;
    }
    buf[len] = '\0';
    *st = STATUS_OK;
    return buf;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <входной_файл> <выходной_файл>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Ошибка: имя входного и выходного файлов не должно совпадать.\n");
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) {
        fprintf(stderr, "Ошибка открытия входного файла: %s\n", argv[1]);
        return 1;
    }

    FILE *out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, "Ошибка открытия выходного файла: %s\n", argv[2]);
        if (fclose(in) != 0) {
            fprintf(stderr, "Ошибка закрытия входного файла.\n");
        }
        return 1;
    }

    size_t line_num = 0;
    status_t global_st = STATUS_OK;
    status_t read_st = STATUS_OK;

    char *line = NULL;
    while ((line = read_line_dynamic(in, &read_st)) != NULL) {
        line_num++;
        
        size_t len = strlen(line);
        if (len == 0) {
            free(line);
            continue;
        }

        const char *p = line;
        node_t *root = NULL;
        status_t st = parse_expression(&p, &root);

        if (st == STATUS_OK) {
            while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
            if (*p != '\0') {
                st = STATUS_INVALID_INPUT;
                node_free(root);
                root = NULL;
            }
        }
        
        if (fprintf(out, "=== Выражение %zu: %s ===\n", line_num, line) < 0) {
            global_st = STATUS_FILE_WRITE_ERROR;
            node_free(root);
            free(line);
            break;
        }

        if (st == STATUS_OK) {
            st = print_tree(out, root, 0);
            node_free(root);
            if (st != STATUS_OK) {
                global_st = st;
                free(line);
                break;
            }
        } else {
            if (fprintf(out, "Ошибка синтаксиса в выражении (код: %d)\n", st) < 0) {
                global_st = STATUS_FILE_WRITE_ERROR;
                free(line);
                break;
            }
        }
        if (fprintf(out, "\n") < 0) {
            global_st = STATUS_FILE_WRITE_ERROR;
            free(line);
            break;
        }
        free(line);
    }

    if (ferror(in)) {
        global_st = STATUS_FILE_READ_ERROR;
    }

    if (fclose(in) != 0) {
        fprintf(stderr, "Ошибка закрытия входного файла.\n");
        global_st = STATUS_FILE_READ_ERROR;
    }

    if (fclose(out) != 0) {
        fprintf(stderr, "Ошибка закрытия выходного файла.\n");
        global_st = STATUS_FILE_WRITE_ERROR;
    }

    if (global_st != STATUS_OK) {
        fprintf(stderr, "Программа завершилась с ошибкой (код: %d)\n", global_st);
        return 1;
    }

    return 0;
}