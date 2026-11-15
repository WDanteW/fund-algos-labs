#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_NO_MEMORY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_FILE_WRITE_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_NOT_FOUND,
    STATUS_OVERFLOW
} status_t;

typedef struct bst_node {
    char *word;
    int count;
    struct bst_node *left;
    struct bst_node *right;
} bst_node_t;

typedef struct {
    char *word;
    int count;
} word_freq_t;

static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char *)malloc(len);
    if (!p) return NULL;
    memcpy(p, s, len);
    return p;
}

static char *read_line_dynamic(FILE *f, status_t *st) {
    size_t cap = 128;
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

static status_t bst_insert(bst_node_t **root, const char *word) {
    if (!root || !word) return STATUS_NULL_POINTER;
    if (!*root) {
        bst_node_t *node = (bst_node_t *)malloc(sizeof(bst_node_t));
        if (!node) return STATUS_NO_MEMORY;
        node->word = my_strdup(word);
        if (!node->word) {
            free(node);
            return STATUS_NO_MEMORY;
        }
        node->count = 1;
        node->left = NULL;
        node->right = NULL;
        *root = node;
        return STATUS_OK;
    }
    int cmp = strcmp(word, (*root)->word);
    if (cmp < 0) {
        return bst_insert(&(*root)->left, word);
    } else if (cmp > 0) {
        return bst_insert(&(*root)->right, word);
    } else {
        if ((*root)->count == INT_MAX) return STATUS_OVERFLOW;
        (*root)->count++;
        return STATUS_OK;
    }
}

static void bst_free(bst_node_t *root) {
    if (!root) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root->word);
    free(root);
}

static int bst_depth(const bst_node_t *root) {
    if (!root) return 0;
    int l_depth = bst_depth(root->left);
    int r_depth = bst_depth(root->right);
    return (l_depth > r_depth ? l_depth : r_depth) + 1;
}

static const bst_node_t *bst_search(const bst_node_t *root, const char *word) {
    if (!root || !word) return NULL;
    int cmp = strcmp(word, root->word);
    if (cmp < 0) return bst_search(root->left, word);
    if (cmp > 0) return bst_search(root->right, word);
    return root;
}

static void bst_collect(const bst_node_t *root, word_freq_t *arr, size_t *idx) {
    if (!root) return;
    bst_collect(root->left, arr, idx);
    arr[*idx].word = root->word;
    arr[*idx].count = root->count;
    (*idx)++;
    bst_collect(root->right, arr, idx);
}

static size_t bst_count_nodes(const bst_node_t *root) {
    if (!root) return 0;
    return 1 + bst_count_nodes(root->left) + bst_count_nodes(root->right);
}

static int compare_word_freq(const void *a, const void *b) {
    const word_freq_t *wf1 = (const word_freq_t *)a;
    const word_freq_t *wf2 = (const word_freq_t *)b;
    if (wf1->count != wf2->count) {
        if (wf1->count > wf2->count) return -1;
        return 1;
    }
    return strcmp(wf1->word, wf2->word);
}

static void bst_find_min_max_len(const bst_node_t *root, const bst_node_t **shortest, const bst_node_t **longest) {
    if (!root) return;
    size_t len = strlen(root->word);
    if (*shortest == NULL || len < strlen((*shortest)->word)) {
        *shortest = root;
    }
    if (*longest == NULL || len > strlen((*longest)->word)) {
        *longest = root;
    }
    bst_find_min_max_len(root->left, shortest, longest);
    bst_find_min_max_len(root->right, shortest, longest);
}

static status_t bst_save_to_file(const bst_node_t *root, FILE *f) {
    if (!f) return STATUS_NULL_POINTER;
    if (!root) {
        if (fprintf(f, "#\n") < 0) return STATUS_FILE_WRITE_ERROR;
        return STATUS_OK;
    }
    if (fprintf(f, "%s\n%d\n", root->word, root->count) < 0) return STATUS_FILE_WRITE_ERROR;
    status_t st = bst_save_to_file(root->left, f);
    if (st != STATUS_OK) return st;
    st = bst_save_to_file(root->right, f);
    return st;
}

static status_t bst_load_from_file(bst_node_t **root, FILE *f) {
    if (!root || !f) return STATUS_NULL_POINTER;
    status_t st = STATUS_OK;
    char *buf = read_line_dynamic(f, &st);
    if (st != STATUS_OK) return st;

    if (strcmp(buf, "#") == 0) {
        free(buf);
        *root = NULL;
        return STATUS_OK;
    }

    char *count_buf = read_line_dynamic(f, &st);
    if (st != STATUS_OK) {
        free(buf);
        return st;
    }

    char *endptr = NULL;
    errno = 0;
    long count_val = strtol(count_buf, &endptr, 10);
    if (endptr == count_buf || *endptr != '\0' || errno != 0 || count_val <= 0 || count_val > INT_MAX) {
        free(buf);
        free(count_buf);
        return STATUS_INVALID_INPUT;
    }
    free(count_buf);

    bst_node_t *node = (bst_node_t *)malloc(sizeof(bst_node_t));
    if (!node) {
        free(buf);
        return STATUS_NO_MEMORY;
    }
    node->word = buf;
    node->count = (int)count_val;
    node->left = NULL;
    node->right = NULL;
    *root = node;

    st = bst_load_from_file(&(*root)->left, f);
    if (st != STATUS_OK) {
        bst_free(*root);
        *root = NULL;
        return st;
    }
    st = bst_load_from_file(&(*root)->right, f);
    if (st != STATUS_OK) {
        bst_free(*root);
        *root = NULL;
        return st;
    }
    return STATUS_OK;
}

static status_t bst_print_statistics(FILE *stream, const bst_node_t *root) {
    if (!stream) return STATUS_NULL_POINTER;

    if (fprintf(stream, "Глубина построенного дерева: %d\n", bst_depth(root)) < 0)
        return STATUS_FILE_WRITE_ERROR;

    size_t total_nodes = bst_count_nodes(root);
    if (total_nodes > 0) {
        word_freq_t *arr = (word_freq_t *)malloc(total_nodes * sizeof(word_freq_t));
        if (!arr) return STATUS_NO_MEMORY;
        size_t idx = 0;
        bst_collect(root, arr, &idx);
        qsort(arr, total_nodes, sizeof(word_freq_t), compare_word_freq);

        const bst_node_t *shortest = NULL;
        const bst_node_t *longest = NULL;
        bst_find_min_max_len(root, &shortest, &longest);

        if (shortest && fprintf(stream, "Самое короткое слово: '%s' (длина: %zu)\n", shortest->word, strlen(shortest->word)) < 0) {
            free(arr);
            return STATUS_FILE_WRITE_ERROR;
        }
        if (longest && fprintf(stream, "Самое длинное слово: '%s' (длина: %zu)\n", longest->word, strlen(longest->word)) < 0) {
            free(arr);
            return STATUS_FILE_WRITE_ERROR;
        }

        if (fprintf(stream, "\nСтатистика частоты слов в тексте:\n") < 0) {
            free(arr);
            return STATUS_FILE_WRITE_ERROR;
        }
        for (size_t i = 0; i < total_nodes; i++) {
            if (fprintf(stream, "  '%s' — %d раз(а)\n", arr[i].word, arr[i].count) < 0) {
                free(arr);
                return STATUS_FILE_WRITE_ERROR;
            }
        }
        free(arr);
    }
    return STATUS_OK;
}

static bool is_separator(char c, int argc, char *argv[]) {
    for (int i = 2; i < argc; i++) {
        if (c == argv[i][0]) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <путь_к_файлу> <сепараторы...>\n", argv[0]);
        fprintf(stderr, "Ошибка: необходимо указать хотя бы один сепаратор.\n");
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (strlen(argv[i]) != 1) {
            fprintf(stderr, "Ошибка: каждый сепаратор должен быть ровно одним символом ('%s').\n", argv[i]);
            return 1;
        }
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла данных.\n");
        return 1;
    }

    bst_node_t *root = NULL;
    size_t buf_cap = 256;
    size_t w_len = 0;
    char *word_buf = (char *)malloc(buf_cap);
    if (!word_buf) {
        fclose(file);
        fprintf(stderr, "Ошибка выделения памяти.\n");
        return 1;
    }

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        char c = (char)ch;
        bool is_sep = is_separator(c, argc, argv);

        if (!is_sep && c == '\r') {
            continue;
        }

        if (is_sep) {
            if (w_len > 0) {
                word_buf[w_len] = '\0';
                status_t st = bst_insert(&root, word_buf);
                if (st != STATUS_OK) {
                    fprintf(stderr, "Ошибка выделения памяти при вставке в дерево.\n");
                    bst_free(root);
                    free(word_buf);
                    if (fclose(file) != 0) {
                        fprintf(stderr, "Ошибка закрытия файла.\n");
                    }
                    return 1;
                }
                w_len = 0;
            }
        } else {
            if (w_len + 1 >= buf_cap) {
                buf_cap *= 2;
                char *new_buf = (char *)realloc(word_buf, buf_cap);
                if (!new_buf) {
                    fprintf(stderr, "Ошибка перераспределения памяти.\n");
                    bst_free(root);
                    free(word_buf);
                    fclose(file);
                    return 1;
                }
                word_buf = new_buf;
            }
            word_buf[w_len++] = c;
        }
    }
    if (w_len > 0) {
        word_buf[w_len] = '\0';
        status_t st = bst_insert(&root, word_buf);
        if (st != STATUS_OK) {
            fprintf(stderr, "Ошибка выделения памяти при вставке в дерево.\n");
            bst_free(root);
            free(word_buf);
            if (fclose(file) != 0) {
                fprintf(stderr, "Ошибка закрытия файла.\n");
            }
            return 1;
        }
    }
    free(word_buf);

    if (fclose(file) != 0) {
        fprintf(stderr, "Предупреждение: ошибка при закрытии файла данных.\n");
    }

    printf("Дерево успешно построено.\n");
    if (bst_print_statistics(stdout, root) != STATUS_OK) {
        fprintf(stderr, "Ошибка вывода статистики.\n");
    }

    bool running = true;
    char line[256];
    while (running) {
        printf("\n=== Меню ===\n");
        printf("1. Узнать частоту конкретного слова\n");
        printf("2. Вывести первые n наиболее частых слов\n");
        printf("3. Сохранить дерево в файл (Пункт C)\n");
        printf("4. Загрузить дерево из файла (Пункт C)\n");
        printf("0. Выход\n");
        printf("Выберите пункт: ");

        if (!fgets(line, sizeof(line), stdin)) break;
        char *endptr = NULL;
        errno = 0;
        long choice = strtol(line, &endptr, 10);
        if (endptr == line || (*endptr != '\n' && *endptr != '\r' && *endptr != '\0') || errno != 0) {
            printf("Неверный ввод.\n");
            continue;
        }

        switch (choice) {
            case 1: {
                printf("Введите слово: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                size_t len = strlen(line);
                while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
                const bst_node_t *res = bst_search(root, line);
                if (res) {
                    printf("Слово '%s' встречается в файле %d раз(а).\n", res->word, res->count);
                } else {
                    printf("Слово не найдено в файле.\n");
                }
                break;
            }
            case 2: {
                printf("Введите число n: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                errno = 0;
                long n = strtol(line, &endptr, 10);
                if (endptr == line || (*endptr != '\n' && *endptr != '\r' && *endptr != '\0') || errno != 0 || n <= 0) {
                    printf("Неверный ввод числа n.\n");
                    break;
                }

                size_t nodes_cnt = bst_count_nodes(root);
                if (nodes_cnt == 0) {
                    printf("Дерево пусто.\n");
                    break;
                }
                word_freq_t *arr = (word_freq_t *)malloc(nodes_cnt * sizeof(word_freq_t));
                if (!arr) break;
                size_t idx = 0;
                bst_collect(root, arr, &idx);
                qsort(arr, nodes_cnt, sizeof(word_freq_t), compare_word_freq);

                printf("Топ %ld слов:\n", n);
                for (long i = 0; i < n && (size_t)i < nodes_cnt; i++) {
                    printf("%ld. %s — %d раз(а)\n", i + 1, arr[i].word, arr[i].count);
                }
                free(arr);
                break;
            }
            case 3: {
                printf("Введите имя файла для сохранения: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                size_t len = strlen(line);
                while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
                FILE *out = fopen(line, "w");
                if (!out) {
                    fprintf(stderr, "Ошибка открытия файла для записи.\n");
                    break;
                }
                if (bst_save_to_file(root, out) == STATUS_OK) {
                    printf("Дерево успешно сохранено.\n");
                } else {
                    fprintf(stderr, "Ошибка записи дерева.\n");
                }
                if (fclose(out) != 0) {
                    fprintf(stderr, "Ошибка закрытия файла сохранения.\n");
                }
                break;
            }
            case 4: {
                printf("Введите имя файла для загрузки: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                size_t len = strlen(line);
                while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
                FILE *in = fopen(line, "r");
                if (!in) {
                    fprintf(stderr, "Ошибка открытия файла для чтения.\n");
                    break;
                }
                bst_node_t *new_root = NULL;
                status_t st = bst_load_from_file(&new_root, in);
                if (fclose(in) != 0) {
                    fprintf(stderr, "Предупреждение: ошибка при закрытии файла загрузки.\n");
                }

                if (st == STATUS_OK) {
                    bst_free(root);
                    root = new_root;
                    printf("Дерево успешно восстановлено из файла сохранения.\n");
                    bst_print_statistics(stdout, root);
                } else {
                    fprintf(stderr, "Ошибка чтения дерева из файла (код: %d).\n", st);
                    bst_free(new_root);
                }
                break;
            }
            case 0:
                running = false;
                break;
            default:
                printf("Неверный пункт меню.\n");
                break;
        }
    }

    bst_free(root);
    return 0;
}