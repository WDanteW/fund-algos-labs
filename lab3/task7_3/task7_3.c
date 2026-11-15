#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_NO_MEMORY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_NOT_FOUND
} status_t;

typedef struct {
    char *surname;
    char *name;
    char *patronymic;
    int day;
    int month;
    int year;
    char gender;
    double income;
} Liver;

typedef struct liver_node {
    Liver data;
    struct liver_node *next;
} liver_node_t;

typedef enum {
    ACTION_ADD = 1,
    ACTION_DELETE,
    ACTION_MODIFY
} action_type_t;

typedef struct undo_record {
    action_type_t type;
    Liver target_data;
    Liver old_data;
    Liver new_data;
    struct undo_record *next;
} undo_record_t;

static char *my_strdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char *)malloc(len);
    if (p == NULL) return NULL;
    memcpy(p, s, len);
    return p;
}

static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int month, int year) {
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month - 1];
}

static status_t validate_alpha_string(const char *str, bool allow_empty) {
    if (str == NULL) return STATUS_NULL_POINTER;
    if (*str == '\0') {
        return allow_empty ? STATUS_OK : STATUS_INVALID_INPUT;
    }
    for (size_t i = 0; str[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)str[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            return STATUS_INVALID_INPUT;
        }
    }
    return STATUS_OK;
}

static status_t validate_date(int day, int month, int year) {
    if (year < 1900 || year > 2026) return STATUS_INVALID_INPUT;
    if (month < 1 || month > 12) return STATUS_INVALID_INPUT;
    int max_d = days_in_month(month, year);
    if (day < 1 || day > max_d) return STATUS_INVALID_INPUT;
    return STATUS_OK;
}

static int compare_ages(int d1, int m1, int y1, int d2, int m2, int y2) {
    if (y1 != y2) return y1 - y2;
    if (m1 != m2) return m1 - m2;
    return d1 - d2;
}

static void free_liver_fields(Liver *l) {
    if (l == NULL) return;
    free(l->surname);
    free(l->name);
    free(l->patronymic);
    l->surname = NULL;
    l->name = NULL;
    l->patronymic = NULL;
}

static status_t copy_liver(const Liver *src, Liver *dest) {
    if (src == NULL || dest == NULL) return STATUS_NULL_POINTER;
    dest->surname = src->surname ? my_strdup(src->surname) : NULL;
    dest->name = src->name ? my_strdup(src->name) : NULL;
    dest->patronymic = src->patronymic ? my_strdup(src->patronymic) : NULL;
    if ((src->surname && !dest->surname) || (src->name && !dest->name) || (src->patronymic && !dest->patronymic)) {
        free_liver_fields(dest);
        return STATUS_NO_MEMORY;
    }
    dest->day = src->day;
    dest->month = src->month;
    dest->year = src->year;
    dest->gender = src->gender;
    dest->income = src->income;
    return STATUS_OK;
}

static bool livers_are_equal(const Liver *l1, const Liver *l2) {
    if (!l1 || !l2) return false;
    if (!l1->surname || !l2->surname || !l1->name || !l2->name || !l1->patronymic || !l2->patronymic) return false;
    return (strcmp(l1->surname, l2->surname) == 0 &&
            strcmp(l1->name, l2->name) == 0 &&
            strcmp(l1->patronymic, l2->patronymic) == 0 &&
            l1->day == l2->day &&
            l1->month == l2->month &&
            l1->year == l2->year &&
            l1->gender == l2->gender &&
            l1->income == l2->income);
}

static void insert_liver_sorted(liver_node_t **head, liver_node_t *new_node) {
    if (head == NULL || new_node == NULL) return;
    if (*head == NULL || compare_ages(new_node->data.year, new_node->data.month, new_node->data.day,
                                      (*head)->data.year, (*head)->data.month, (*head)->data.day) < 0) {
        new_node->next = *head;
        *head = new_node;
        return;
    }
    liver_node_t *curr = *head;
    while (curr->next != NULL &&
           compare_ages(curr->next->data.year, curr->next->data.month, curr->next->data.day,
                        new_node->data.year, new_node->data.month, new_node->data.day) <= 0) {
        curr = curr->next;
    }
    new_node->next = curr->next;
    curr->next = new_node;
}

static void free_list(liver_node_t *head) {
    while (head != NULL) {
        liver_node_t *tmp = head;
        head = head->next;
        free_liver_fields(&tmp->data);
        free(tmp);
    }
}

static void free_undo_stack(undo_record_t *head) {
    while (head != NULL) {
        undo_record_t *tmp = head;
        head = head->next;
        free_liver_fields(&tmp->target_data);
        free_liver_fields(&tmp->old_data);
        free_liver_fields(&tmp->new_data);
        free(tmp);
    }
}

static status_t push_undo(undo_record_t **undo_stack, action_type_t type,
                          const Liver *target, const Liver *old_d, const Liver *new_d) {
    if (undo_stack == NULL) return STATUS_NULL_POINTER;
    undo_record_t *rec = (undo_record_t *)malloc(sizeof(undo_record_t));
    if (rec == NULL) return STATUS_NO_MEMORY;

    rec->type = type;
    rec->target_data.surname = NULL; rec->target_data.name = NULL; rec->target_data.patronymic = NULL;
    rec->old_data.surname = NULL; rec->old_data.name = NULL; rec->old_data.patronymic = NULL;
    rec->new_data.surname = NULL; rec->new_data.name = NULL; rec->new_data.patronymic = NULL;

    if (target && copy_liver(target, &rec->target_data) != STATUS_OK) {
        free(rec);
        return STATUS_NO_MEMORY;
    }
    if (old_d && copy_liver(old_d, &rec->old_data) != STATUS_OK) {
        free_liver_fields(&rec->target_data);
        free(rec);
        return STATUS_NO_MEMORY;
    }
    if (new_d && copy_liver(new_d, &rec->new_data) != STATUS_OK) {
        free_liver_fields(&rec->target_data);
        free_liver_fields(&rec->old_data);
        free(rec);
        return STATUS_NO_MEMORY;
    }

    rec->next = *undo_stack;
    *undo_stack = rec;
    return STATUS_OK;
}

static status_t read_livers_from_file(const char *filename, liver_node_t **head) {
    if (filename == NULL || head == NULL) return STATUS_NULL_POINTER;
    FILE *file = fopen(filename, "r");
    if (file == NULL) return STATUS_FILE_OPEN_ERROR;

    char line[512];
    status_t status = STATUS_OK;

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        }
        if (len == 0) continue;

        char surname[128], name[128], patronymic[128], gender_str[16];
        int d = 0, m = 0, y = 0;
        double income = 0.0;

        char d_str[32], m_str[32], y_str[32], inc_str[64];
        int parsed = sscanf(line, "%127s %127s %127s %31s %31s %31s %15s %63s",
                            surname, name, patronymic, d_str, m_str, y_str, gender_str, inc_str);
        if (parsed != 8) {
            parsed = sscanf(line, "%127s %127s %31s %31s %31s %15s %63s",
                            surname, name, d_str, m_str, y_str, gender_str, inc_str);
            if (parsed == 7) {
                patronymic[0] = '\0';
            } else {
                status = STATUS_INVALID_INPUT;
                break;
            }
        }

        if (strcmp(patronymic, "-") == 0) {
            patronymic[0] = '\0';
        }

        char *endptr = NULL;
        d = (int)strtol(d_str, &endptr, 10);
        if (endptr == d_str || *endptr != '\0') { status = STATUS_INVALID_INPUT; break; }
        m = (int)strtol(m_str, &endptr, 10);
        if (endptr == m_str || *endptr != '\0') { status = STATUS_INVALID_INPUT; break; }
        y = (int)strtol(y_str, &endptr, 10);
        if (endptr == y_str || *endptr != '\0') { status = STATUS_INVALID_INPUT; break; }
        income = strtod(inc_str, &endptr);
        if (endptr == inc_str || *endptr != '\0') { status = STATUS_INVALID_INPUT; break; }

        if (validate_alpha_string(surname, false) != STATUS_OK ||
            validate_alpha_string(name, false) != STATUS_OK ||
            validate_alpha_string(patronymic, true) != STATUS_OK ||
            validate_date(d, m, y) != STATUS_OK ||
            (gender_str[0] != 'M' && gender_str[0] != 'W') ||
            income < 0.0) {
            status = STATUS_INVALID_INPUT;
            break;
        }

        liver_node_t *node = (liver_node_t *)malloc(sizeof(liver_node_t));
        if (node == NULL) {
            status = STATUS_NO_MEMORY;
            break;
        }

        node->data.surname = my_strdup(surname);
        node->data.name = my_strdup(name);
        node->data.patronymic = patronymic[0] != '\0' ? my_strdup(patronymic) : my_strdup("");
        node->data.day = d;
        node->data.month = m;
        node->data.year = y;
        node->data.gender = gender_str[0];
        node->data.income = income;
        node->next = NULL;

        if (!node->data.surname || !node->data.name || !node->data.patronymic) {
            free_liver_fields(&node->data);
            free(node);
            status = STATUS_NO_MEMORY;
            break;
        }

        insert_liver_sorted(head, node);
    }

    if (status == STATUS_OK && ferror(file)) {
        status = STATUS_FILE_READ_ERROR;
    }

    fclose(file);
    return status;
}

static status_t write_livers_to_file(const char *filename, const liver_node_t *head) {
    if (filename == NULL) return STATUS_NULL_POINTER;
    FILE *file = fopen(filename, "w");
    if (file == NULL) return STATUS_FILE_OPEN_ERROR;

    const liver_node_t *curr = head;
    while (curr != NULL) {
        if (fprintf(file, "%s %s %s %d %d %d %c %.2f\n",
                    curr->data.surname,
                    curr->data.name,
                    curr->data.patronymic[0] != '\0' ? curr->data.patronymic : "-",
                    curr->data.day,
                    curr->data.month,
                    curr->data.year,
                    curr->data.gender,
                    curr->data.income) < 0) {
            fclose(file);
            return STATUS_FILE_READ_ERROR;
        }
        curr = curr->next;
    }

    if (ferror(file)) {
        fclose(file);
        return STATUS_FILE_READ_ERROR;
    }

    if (fclose(file) != 0) {
        return STATUS_FILE_READ_ERROR;
    }

    return STATUS_OK;
}

static void print_liver(FILE *stream, const Liver *l) {
    if (stream == NULL || l == NULL) return;
    fprintf(stream, "ФИО: %s %s %s | Дата рождения: %02d.%02d.%d | Пол: %c | Доход: %.2f\n",
            l->surname ? l->surname : "",
            l->name ? l->name : "",
            (l->patronymic && l->patronymic[0] != '\0') ? l->patronymic : "-",
            l->day, l->month, l->year, l->gender, l->income);
}

static void print_list(FILE *stream, const liver_node_t *head) {
    if (stream == NULL) return;
    if (head == NULL) {
        fprintf(stream, "Список жителей пуст.\n");
        return;
    }
    const liver_node_t *curr = head;
    size_t idx = 1;
    while (curr != NULL) {
        fprintf(stream, "%zu. ", idx++);
        print_liver(stream, &curr->data);
        curr = curr->next;
    }
}

static status_t add_liver(liver_node_t **head, undo_record_t **undo_stack, const Liver *new_l) {
    if (head == NULL || undo_stack == NULL || new_l == NULL) return STATUS_NULL_POINTER;

    status_t st = push_undo(undo_stack, ACTION_ADD, new_l, NULL, NULL);
    if (st != STATUS_OK) return st;

    liver_node_t *node = (liver_node_t *)malloc(sizeof(liver_node_t));
    if (node == NULL) {
        undo_record_t *top = *undo_stack;
        *undo_stack = top->next;
        free_liver_fields(&top->target_data);
        free(top);
        return STATUS_NO_MEMORY;
    }

    if (copy_liver(new_l, &node->data) != STATUS_OK) {
        free(node);
        undo_record_t *top = *undo_stack;
        *undo_stack = top->next;
        free_liver_fields(&top->target_data);
        free(top);
        return STATUS_NO_MEMORY;
    }
    node->next = NULL;

    insert_liver_sorted(head, node);
    return STATUS_OK;
}

static status_t delete_liver_by_criteria(liver_node_t **head, undo_record_t **undo_stack,
                                         const char *surname, const char *name) {
    if (head == NULL || undo_stack == NULL || surname == NULL || name == NULL) return STATUS_NULL_POINTER;

    liver_node_t dummy;
    dummy.next = *head;
    liver_node_t *prev = &dummy;
    liver_node_t *curr = *head;
    liver_node_t *target_node = NULL;

    while (curr != NULL) {
        if (strcmp(curr->data.surname, surname) == 0 && strcmp(curr->data.name, name) == 0) {
            target_node = curr;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if (target_node == NULL) return STATUS_NOT_FOUND;

    status_t st = push_undo(undo_stack, ACTION_DELETE, &target_node->data, NULL, NULL);
    if (st != STATUS_OK) return st;

    prev->next = target_node->next;
    *head = dummy.next;

    free_liver_fields(&target_node->data);
    free(target_node);
    return STATUS_OK;
}

static status_t modify_liver(liver_node_t *head, undo_record_t **undo_stack,
                             const char *surname, const char *name, const Liver *updated_l) {
    if (head == NULL || undo_stack == NULL || surname == NULL || name == NULL || updated_l == NULL)
        return STATUS_NULL_POINTER;

    liver_node_t *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->data.surname, surname) == 0 && strcmp(curr->data.name, name) == 0) {
            Liver old_data;
            if (copy_liver(&curr->data, &old_data) != STATUS_OK) return STATUS_NO_MEMORY;

            status_t st = push_undo(undo_stack, ACTION_MODIFY, NULL, &old_data, updated_l);
            if (st != STATUS_OK) {
                free_liver_fields(&old_data);
                return st;
            }

            Liver new_copied;
            if (copy_liver(updated_l, &new_copied) != STATUS_OK) {
                free_liver_fields(&old_data);
                return STATUS_NO_MEMORY;
            }

            free_liver_fields(&curr->data);
            curr->data = new_copied;
            free_liver_fields(&old_data);
            return STATUS_OK;
        }
        curr = curr->next;
    }
    return STATUS_NOT_FOUND;
}

static status_t perform_undo(liver_node_t **head, undo_record_t **undo_stack) {
    if (head == NULL || undo_stack == NULL || *undo_stack == NULL) return STATUS_NOT_FOUND;

    undo_record_t *rec = *undo_stack;

    if (rec->type == ACTION_ADD) {
        liver_node_t dummy;
        dummy.next = *head;
        liver_node_t *prev = &dummy;
        liver_node_t *curr = *head;
        bool found = false;
        while (curr != NULL) {
            if (livers_are_equal(&curr->data, &rec->target_data)) {
                found = true;
                break;
            }
            prev = curr;
            curr = curr->next;
        }
        if (!found) return STATUS_NOT_FOUND;

        *undo_stack = rec->next;
        prev->next = curr->next;
        free_liver_fields(&curr->data);
        free(curr);
        *head = dummy.next;
    } else if (rec->type == ACTION_DELETE) {
        liver_node_t *node = (liver_node_t *)malloc(sizeof(liver_node_t));
        if (node == NULL) {
            return STATUS_NO_MEMORY;
        }
        if (copy_liver(&rec->target_data, &node->data) != STATUS_OK) {
            free(node);
            return STATUS_NO_MEMORY;
        }
        *undo_stack = rec->next;
        node->next = NULL;
        insert_liver_sorted(head, node);
    } else if (rec->type == ACTION_MODIFY) {
        liver_node_t *curr = *head;
        bool found = false;
        while (curr != NULL) {
            if (strcmp(curr->data.surname, rec->new_data.surname) == 0 &&
                strcmp(curr->data.name, rec->new_data.name) == 0) {
                found = true;
                break;
            }
            curr = curr->next;
        }
        if (!found) return STATUS_NOT_FOUND;

        Liver restored_old;
        if (copy_liver(&rec->old_data, &restored_old) != STATUS_OK) {
            return STATUS_NO_MEMORY;
        }
        *undo_stack = rec->next;
        free_liver_fields(&curr->data);
        curr->data = restored_old;
    }

    free_liver_fields(&rec->target_data);
    free_liver_fields(&rec->old_data);
    free_liver_fields(&rec->new_data);
    free(rec);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <путь_к_файлу_данных>\n", argv[0]);
        return 1;
    }

    liver_node_t *head = NULL;
    undo_record_t *undo_stack = NULL;
    size_t total_modifications_ever = 0;
    size_t undo_operations_count = 0;

    status_t st = read_livers_from_file(argv[1], &head);
    if (st != STATUS_OK) {
        fprintf(stderr, "Ошибка чтения входного файла (код: %d).\n", st);
        free_list(head);
        return 1;
    }

    bool running = true;
    char input_buf[128];

    while (running) {
        size_t max_undo_allowed = total_modifications_ever / 2;
        size_t undo_limit = (max_undo_allowed > undo_operations_count) ?
                            (max_undo_allowed - undo_operations_count) : 0;

        printf("\n=== Управление жителями поселения ===\n");
        printf("Всего модификаций: %zu | Выполнено Undo: %zu | Доступно Undo (⌊N/2⌋): %zu\n",
               total_modifications_ever, undo_operations_count, undo_limit);
        printf("1. Вывести список жителей\n");
        printf("2. Добавить жителя\n");
        printf("3. Удалить жителя по Фамилии и Имени\n");
        printf("4. Изменить данные жителя\n");
        printf("5. Выгрузить данные в файл\n");
        printf("6. Отменить модификацию (Undo)\n");
        printf("0. Выход\n");
        printf("Выберите пункт меню: ");

        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) break;
        if (input_buf[0] == '\n' || input_buf[0] == '\r' || input_buf[0] == '\0') {
            fprintf(stderr, "Неверный ввод.\n");
            continue;
        }

        char *endptr = NULL;
        long choice = strtol(input_buf, &endptr, 10);
        while (*endptr != '\0' && isspace((unsigned char)*endptr)) endptr++;
        if (endptr == input_buf || *endptr != '\0') {
            fprintf(stderr, "Неверный ввод.\n");
            continue;
        }

        switch (choice) {
            case 1: {
                print_list(stdout, head);
                break;
            }
            case 2: {
                Liver l;
                char surname[128], name[128], patronymic[128], gender_buf[16];
                printf("Введите фамилию: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                size_t len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", surname) != 1) break;

                printf("Введите имя: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", name) != 1) break;

                printf("Введите отчество (или '-' если нет): ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", patronymic) != 1) break;
                if (strcmp(patronymic, "-") == 0) patronymic[0] = '\0';

                printf("Введите дату рождения (день месяц год через пробел): ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                char d_str[32], m_str[32], y_str[32];
                if (sscanf(input_buf, "%31s %31s %31s", d_str, m_str, y_str) != 3) {
                    fprintf(stderr, "Ошибка ввода даты.\n");
                    break;
                }
                l.day = (int)strtol(d_str, &endptr, 10);
                if (endptr == d_str || *endptr != '\0') { fprintf(stderr, "Неверный день.\n"); break; }
                l.month = (int)strtol(m_str, &endptr, 10);
                if (endptr == m_str || *endptr != '\0') { fprintf(stderr, "Неверный месяц.\n"); break; }
                l.year = (int)strtol(y_str, &endptr, 10);
                if (endptr == y_str || *endptr != '\0') { fprintf(stderr, "Неверный год.\n"); break; }

                printf("Введите пол (M/W): ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%15s", gender_buf) != 1) break;

                printf("Введите средний доход: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                char inc_str[64];
                if (sscanf(input_buf, "%63s", inc_str) != 1) {
                    fprintf(stderr, "Ошибка ввода дохода.\n");
                    break;
                }
                l.income = strtod(inc_str, &endptr);
                if (endptr == inc_str || *endptr != '\0') {
                    fprintf(stderr, "Неверный доход.\n");
                    break;
                }

                l.surname = surname;
                l.name = name;
                l.patronymic = patronymic;
                l.gender = gender_buf[0];

                if (validate_alpha_string(surname, false) != STATUS_OK ||
                    validate_alpha_string(name, false) != STATUS_OK ||
                    validate_alpha_string(patronymic, true) != STATUS_OK ||
                    validate_date(l.day, l.month, l.year) != STATUS_OK ||
                    (l.gender != 'M' && l.gender != 'W') || l.income < 0) {
                    fprintf(stderr, "Ошибка: неверные данные.\n");
                    break;
                }

                st = add_liver(&head, &undo_stack, &l);
                if (st == STATUS_OK) {
                    total_modifications_ever++;
                    printf("Житель успешно добавлен.\n");
                } else {
                    fprintf(stderr, "Ошибка добавления (код: %d).\n", st);
                }
                break;
            }
            case 3: {
                char surname[128], name[128];
                printf("Введите фамилию удаляемого жителя: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                size_t len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", surname) != 1) break;

                printf("Введите имя удаляемого жителя: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", name) != 1) break;

                if (validate_alpha_string(surname, false) != STATUS_OK ||
                    validate_alpha_string(name, false) != STATUS_OK) {
                    fprintf(stderr, "Ошибка: фамилия и имя должны содержать только буквы.\n");
                    break;
                }

                st = delete_liver_by_criteria(&head, &undo_stack, surname, name);
                if (st == STATUS_OK) {
                    total_modifications_ever++;
                    printf("Житель успешно удален.\n");
                } else {
                    fprintf(stderr, "Житель не найден.\n");
                }
                break;
            }
            case 4: {
                char surname[128], name[128];
                printf("Введите фамилию изменяемого жителя: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                size_t len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", surname) != 1) break;

                printf("Введите имя изменяемого жителя: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", name) != 1) break;

                if (validate_alpha_string(surname, false) != STATUS_OK ||
                    validate_alpha_string(name, false) != STATUS_OK) {
                    fprintf(stderr, "Ошибка: фамилия и имя должны содержать только буквы.\n");
                    break;
                }

                Liver updated;
                char u_surname[128], u_name[128], u_patr[128], g_buf[16];
                printf("Новая фамилия: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", u_surname) != 1) break;

                printf("Новое имя: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", u_name) != 1) break;

                printf("Новое отчество ('-' если нет): ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%127s", u_patr) != 1) break;
                if (strcmp(u_patr, "-") == 0) u_patr[0] = '\0';

                printf("Новая дата (д м г): ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                char d_str[32], m_str[32], y_str[32];
                if (sscanf(input_buf, "%31s %31s %31s", d_str, m_str, y_str) != 3) {
                    fprintf(stderr, "Ошибка ввода даты.\n");
                    break;
                }
                updated.day = (int)strtol(d_str, &endptr, 10);
                if (endptr == d_str || *endptr != '\0') { fprintf(stderr, "Неверный день.\n"); break; }
                updated.month = (int)strtol(m_str, &endptr, 10);
                if (endptr == m_str || *endptr != '\0') { fprintf(stderr, "Неверный месяц.\n"); break; }
                updated.year = (int)strtol(y_str, &endptr, 10);
                if (endptr == y_str || *endptr != '\0') { fprintf(stderr, "Неверный год.\n"); break; }

                printf("Новый пол (M/W): ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%15s", g_buf) != 1) break;

                printf("Новый доход: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                char inc_str[64];
                if (sscanf(input_buf, "%63s", inc_str) != 1) {
                    fprintf(stderr, "Ошибка ввода дохода.\n");
                    break;
                }
                updated.income = strtod(inc_str, &endptr);
                if (endptr == inc_str || *endptr != '\0') {
                    fprintf(stderr, "Неверный доход.\n");
                    break;
                }

                updated.surname = u_surname;
                updated.name = u_name;
                updated.patronymic = u_patr;
                updated.gender = g_buf[0];

                if (validate_alpha_string(u_surname, false) != STATUS_OK ||
                    validate_alpha_string(u_name, false) != STATUS_OK ||
                    validate_alpha_string(u_patr, true) != STATUS_OK ||
                    validate_date(updated.day, updated.month, updated.year) != STATUS_OK ||
                    (updated.gender != 'M' && updated.gender != 'W') || updated.income < 0) {
                    fprintf(stderr, "Ошибка: неверные новые данные.\n");
                    break;
                }

                st = modify_liver(head, &undo_stack, surname, name, &updated);
                if (st == STATUS_OK) {
                    total_modifications_ever++;
                    printf("Данные успешно изменены.\n");
                } else {
                    fprintf(stderr, "Житель не найден или ошибка данных.\n");
                }
                break;
            }
            case 5: {
                char out_path[256];
                printf("Введите путь для выгрузки в файл: ");
                if (!fgets(input_buf, sizeof(input_buf), stdin)) break;
                size_t len = strlen(input_buf);
                while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) input_buf[--len] = '\0';
                if (sscanf(input_buf, "%255s", out_path) != 1) break;

                st = write_livers_to_file(out_path, head);
                if (st == STATUS_OK) printf("Данные успешно выгружены в '%s'.\n", out_path);
                else fprintf(stderr, "Ошибка записи в файл.\n");
                break;
            }
            case 6: {
                if (undo_limit == 0) {
                    printf("Нет доступных отмен для выполнения (лимит ⌊N/2⌋ исчерпан).\n");
                    break;
                }
                st = perform_undo(&head, &undo_stack);
                if (st == STATUS_OK) {
                    undo_operations_count++;
                    printf("Последнее изменение успешно отменено.\n");
                } else {
                    fprintf(stderr, "Ошибка отмены.\n");
                }
                break;
            }
            case 0: {
                running = false;
                break;
            }
            default:
                printf("Неверный выбор меню.\n");
        }
    }

    free_list(head);
    free_undo_stack(undo_stack);
    return 0;
}