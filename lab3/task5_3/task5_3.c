#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_NO_MEMORY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_NOT_FOUND,
    STATUS_OVERFLOW
} status_t;

typedef struct {
    unsigned int id;
    char *name;
    char *surname;
    char *group;
    unsigned char *grades;
    size_t grades_count;
} student_t;

typedef struct {
    student_t *data;
    size_t size;
    size_t capacity;
} student_vector_t;

typedef struct {
    student_t **data;
    size_t size;
    size_t capacity;
} student_ptr_vector_t;

static char *my_strdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p == NULL) return NULL;
    memcpy(p, s, len);
    return p;
}

static status_t vector_init(student_vector_t *vec, size_t initial_capacity) {
    if (vec == NULL) return STATUS_NULL_POINTER;
    if (initial_capacity == 0) initial_capacity = 4;

    vec->data = (student_t *)malloc(initial_capacity * sizeof(student_t));
    if (vec->data == NULL) return STATUS_NO_MEMORY;

    vec->size = 0;
    vec->capacity = initial_capacity;
    return STATUS_OK;
}

static void student_free(student_t *student) {
    if (student == NULL) return;
    free(student->name);
    free(student->surname);
    free(student->group);
    free(student->grades);
    student->name = NULL;
    student->surname = NULL;
    student->group = NULL;
    student->grades = NULL;
}

static void vector_free(student_vector_t *vec) {
    if (vec == NULL) return;
    for (size_t i = 0; i < vec->size; ++i) {
        student_free(&vec->data[i]);
    }
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

static status_t vector_push(student_vector_t *vec, const student_t *student) {
    if (vec == NULL || student == NULL) return STATUS_NULL_POINTER;

    if (vec->size >= vec->capacity) {
        size_t new_capacity = vec->capacity * 2;
        student_t *grown = (student_t *)realloc(vec->data, new_capacity * sizeof(student_t));
        if (grown == NULL) return STATUS_NO_MEMORY;
        vec->data = grown;
        vec->capacity = new_capacity;
    }

    vec->data[vec->size] = *student;
    vec->size++;
    return STATUS_OK;
}

static status_t ptr_vector_init(student_ptr_vector_t *vec, size_t initial_capacity) {
    if (vec == NULL) return STATUS_NULL_POINTER;
    if (initial_capacity == 0) initial_capacity = 4;

    vec->data = (student_t **)malloc(initial_capacity * sizeof(student_t *));
    if (vec->data == NULL) return STATUS_NO_MEMORY;

    vec->size = 0;
    vec->capacity = initial_capacity;
    return STATUS_OK;
}

static void ptr_vector_free(student_ptr_vector_t *vec) {
    if (vec == NULL) return;
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

static status_t ptr_vector_push(student_ptr_vector_t *vec, student_t *student) {
    if (vec == NULL || student == NULL) return STATUS_NULL_POINTER;

    if (vec->size >= vec->capacity) {
        size_t new_capacity = vec->capacity * 2;
        student_t **grown = (student_t **)realloc(vec->data, new_capacity * sizeof(student_t *));
        if (grown == NULL) return STATUS_NO_MEMORY;
        vec->data = grown;
        vec->capacity = new_capacity;
    }

    vec->data[vec->size] = student;
    vec->size++;
    return STATUS_OK;
}

static status_t parse_unsigned_int(const char *str, unsigned int *result) {
    if (str == NULL || result == NULL || *str == '\0') return STATUS_INVALID_INPUT;

    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);

    while (*endptr != '\0' && (*endptr == '\n' || *endptr == '\r' || isspace((unsigned char)*endptr))) {
        endptr++;
    }

    if (endptr == str || *endptr != '\0' || val < 0 || val > UINT_MAX) {
        return STATUS_INVALID_INPUT;
    }

    *result = (unsigned int)val;
    return STATUS_OK;
}

static status_t validate_alpha_string(const char *str) {
    if (str == NULL || *str == '\0') return STATUS_INVALID_INPUT;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            return STATUS_INVALID_INPUT;
        }
    }
    return STATUS_OK;
}

static status_t validate_group_string(const char *str) {
    if (str == NULL || *str == '\0') return STATUS_INVALID_INPUT;
    return STATUS_OK;
}

static status_t parse_grades(const char *str, unsigned char **grades, size_t *count) {
    if (str == NULL || grades == NULL || count == NULL) return STATUS_NULL_POINTER;

    unsigned char *arr = (unsigned char *)malloc(5 * sizeof(unsigned char));
    if (arr == NULL) return STATUS_NO_MEMORY;

    const char *ptr = str;
    for (size_t i = 0; i < 5; ++i) {
        while (*ptr != '\0' && isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0') {
            free(arr);
            return STATUS_INVALID_INPUT;
        }

        char *endptr = NULL;
        long val = strtol(ptr, &endptr, 10);
        if (endptr == ptr || val < 1 || val > 5) {
            free(arr);
            return STATUS_INVALID_INPUT;
        }
        arr[i] = (unsigned char)val;
        ptr = endptr;
    }

    while (*ptr != '\0') {
        if (!isspace((unsigned char)*ptr)) {
            free(arr);
            return STATUS_INVALID_INPUT;
        }
        ptr++;
    }

    *grades = arr;
    *count = 5;
    return STATUS_OK;
}

static status_t read_students_from_file(const char *filename, student_vector_t *vec) {
    if (filename == NULL || vec == NULL) return STATUS_NULL_POINTER;

    FILE *file = fopen(filename, "r");
    if (file == NULL) return STATUS_FILE_OPEN_ERROR;

    char line[512];
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        }
        if (len == 0) continue;

        char id_str[64], name_str[128], surname_str[128], group_str[128];

        if (sscanf(line, "%63s %127s %127s %127s", id_str, name_str, surname_str, group_str) != 4) {
            continue;
        }

        const char *ptr = line;
        for (int i = 0; i < 4; ++i) {
            while (*ptr != '\0' && !isspace((unsigned char)*ptr)) ptr++;
            while (*ptr != '\0' && isspace((unsigned char)*ptr)) ptr++;
        }
        const char *grades_ptr = ptr;

        unsigned int id;
        if (parse_unsigned_int(id_str, &id) != STATUS_OK ||
            validate_alpha_string(name_str) != STATUS_OK ||
            validate_alpha_string(surname_str) != STATUS_OK ||
            validate_group_string(group_str) != STATUS_OK ||
            *grades_ptr == '\0') {
            continue;
        }

        unsigned char *grades = NULL;
        size_t grades_count = 0;
        if (parse_grades(grades_ptr, &grades, &grades_count) != STATUS_OK) {
            continue;
        }

        student_t st;
        st.id = id;
        st.name = my_strdup(name_str);
        st.surname = my_strdup(surname_str);
        st.group = my_strdup(group_str);
        st.grades = grades;
        st.grades_count = grades_count;

        if (st.name == NULL || st.surname == NULL || st.group == NULL) {
            student_free(&st);
            fclose(file);
            return STATUS_NO_MEMORY;
        }

        status_t st_push = vector_push(vec, &st);
        if (st_push != STATUS_OK) {
            student_free(&st);
            fclose(file);
            return st_push;
        }
    }

    if (ferror(file)) {
        fclose(file);
        return STATUS_FILE_READ_ERROR;
    }

    fclose(file);
    return STATUS_OK;
}

static status_t find_by_id(const student_vector_t *vec, unsigned int id, student_ptr_vector_t *result) {
    if (vec == NULL || result == NULL) return STATUS_NULL_POINTER;
    for (size_t i = 0; i < vec->size; ++i) {
        if (vec->data[i].id == id) {
            status_t st = ptr_vector_push(result, &vec->data[i]);
            if (st != STATUS_OK) return st;
        }
    }
    return result->size > 0 ? STATUS_OK : STATUS_NOT_FOUND;
}

static status_t find_by_surname(const student_vector_t *vec, const char *surname, student_ptr_vector_t *result) {
    if (vec == NULL || surname == NULL || result == NULL) return STATUS_NULL_POINTER;
    for (size_t i = 0; i < vec->size; ++i) {
        if (strcmp(vec->data[i].surname, surname) == 0) {
            status_t st = ptr_vector_push(result, &vec->data[i]);
            if (st != STATUS_OK) return st;
        }
    }
    return result->size > 0 ? STATUS_OK : STATUS_NOT_FOUND;
}

static status_t find_by_name(const student_vector_t *vec, const char *name, student_ptr_vector_t *result) {
    if (vec == NULL || name == NULL || result == NULL) return STATUS_NULL_POINTER;
    for (size_t i = 0; i < vec->size; ++i) {
        if (strcmp(vec->data[i].name, name) == 0) {
            status_t st = ptr_vector_push(result, &vec->data[i]);
            if (st != STATUS_OK) return st;
        }
    }
    return result->size > 0 ? STATUS_OK : STATUS_NOT_FOUND;
}

static status_t find_by_group(const student_vector_t *vec, const char *group, student_ptr_vector_t *result) {
    if (vec == NULL || group == NULL || result == NULL) return STATUS_NULL_POINTER;
    for (size_t i = 0; i < vec->size; ++i) {
        if (strcmp(vec->data[i].group, group) == 0) {
            status_t st = ptr_vector_push(result, &vec->data[i]);
            if (st != STATUS_OK) return st;
        }
    }
    return result->size > 0 ? STATUS_OK : STATUS_NOT_FOUND;
}

static int compare_by_id(const void *a, const void *b) {
    const student_t *s1 = (const student_t *)a;
    const student_t *s2 = (const student_t *)b;
    if (s1->id < s2->id) return -1;
    if (s1->id > s2->id) return 1;
    return 0;
}

static int compare_by_surname(const void *a, const void *b) {
    const student_t *s1 = (const student_t *)a;
    const student_t *s2 = (const student_t *)b;
    return strcmp(s1->surname, s2->surname);
}

static int compare_by_name(const void *a, const void *b) {
    const student_t *s1 = (const student_t *)a;
    const student_t *s2 = (const student_t *)b;
    return strcmp(s1->name, s2->name);
}

static int compare_by_group(const void *a, const void *b) {
    const student_t *s1 = (const student_t *)a;
    const student_t *s2 = (const student_t *)b;
    return strcmp(s1->group, s2->group);
}

static double calculate_student_average(const student_t *student) {
    if (student == NULL || student->grades_count == 0) return 0.0;
    unsigned int sum = 0;
    for (size_t i = 0; i < student->grades_count; ++i) {
        sum += student->grades[i];
    }
    return (double)sum / (double)student->grades_count;
}

static void print_students(FILE *stream, const student_vector_t *vec) {
    if (stream == NULL || vec == NULL) return;
    for (size_t i = 0; i < vec->size; ++i) {
        fprintf(stream, "ID: %u | ФИО: %s %s | Группа: %s | Оценки: ",
                vec->data[i].id, vec->data[i].surname, vec->data[i].name, vec->data[i].group);
        for (size_t g = 0; g < vec->data[i].grades_count; ++g) {
            fprintf(stream, "%u ", vec->data[i].grades[g]);
        }
        fprintf(stream, "| Средний балл: %.2f\n", calculate_student_average(&vec->data[i]));
    }
}

static void print_ptr_students(FILE *stream, const student_ptr_vector_t *vec) {
    if (stream == NULL || vec == NULL) return;
    for (size_t i = 0; i < vec->size; ++i) {
        const student_t *s = vec->data[i];
        if (s == NULL) continue;
        fprintf(stream, "ID: %u | ФИО: %s %s | Группа: %s | Оценки: ",
                s->id, s->surname, s->name, s->group);
        for (size_t g = 0; g < s->grades_count; ++g) {
            fprintf(stream, "%u ", s->grades[g]);
        }
        fprintf(stream, "| Средний балл: %.2f\n", calculate_student_average(s));
    }
}

static status_t trace_student_by_id(const student_vector_t *vec, unsigned int id, const char *trace_filename) {
    if (vec == NULL || trace_filename == NULL) return STATUS_NULL_POINTER;

    student_ptr_vector_t found;
    status_t st = ptr_vector_init(&found, 2);
    if (st != STATUS_OK) return st;

    st = find_by_id(vec, id, &found);
    if (st != STATUS_OK) {
        ptr_vector_free(&found);
        return st;
    }

    FILE *file = fopen(trace_filename, "w");
    if (file == NULL) {
        ptr_vector_free(&found);
        return STATUS_FILE_OPEN_ERROR;
    }

    fprintf(file, "=== Данные студента с ID: %u ===\n", id);
    for (size_t i = 0; i < found.size; ++i) {
        const student_t *s = found.data[i];
        fprintf(file, "ФИО: %s %s\n", s->surname, s->name);
        fprintf(file, "Группа: %s\n", s->group);
        fprintf(file, "Средний балл за экзамены: %.2f\n", calculate_student_average(s));
    }

    fclose(file);
    ptr_vector_free(&found);
    return STATUS_OK;
}

static status_t trace_above_average_students(const student_vector_t *vec, const char *trace_filename) {
    if (vec == NULL || vec->size == 0 || trace_filename == NULL) return STATUS_NULL_POINTER;

    double total_sum = 0.0;
    size_t total_grades = 0;
    for (size_t i = 0; i < vec->size; ++i) {
        for (size_t g = 0; g < vec->data[i].grades_count; ++g) {
            total_sum += vec->data[i].grades[g];
            total_grades++;
        }
    }
    if (total_grades == 0) return STATUS_INVALID_INPUT;
    double overall_average = total_sum / (double)total_grades;

    FILE *file = fopen(trace_filename, "w");
    if (file == NULL) return STATUS_FILE_OPEN_ERROR;

    fprintf(file, "=== Студенты со средним баллом выше общего (%.2f) ===\n", overall_average);
    for (size_t i = 0; i < vec->size; ++i) {
        double st_avg = calculate_student_average(&vec->data[i]);
        if (st_avg > overall_average) {
            fprintf(file, "Фамилия: %s, Имя: %s (Средний балл: %.2f)\n",
                    vec->data[i].surname, vec->data[i].name, st_avg);
        }
    }

    fclose(file);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <путь_к_файлу_данных> <путь_к_трассировочному_файлу>\n", argv[0]);
        return 1;
    }

    const char *data_filename = argv[1];
    const char *trace_filename = argv[2];

    student_vector_t students;
    if (vector_init(&students, 10) != STATUS_OK) {
        fprintf(stderr, "Ошибка: не удалось выделить память.\n");
        return 1;
    }

    status_t status = read_students_from_file(data_filename, &students);
    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка чтения файла данных (статус: %d).\n", status);
        vector_free(&students);
        return 1;
    }

    if (students.size == 0) {
        printf("Файл пуст или не содержит корректных записей о студентах.\n");
        vector_free(&students);
        return 0;
    }

    bool running = true;

    while (running) {
        printf("\n=== Меню управления студентами ===\n");
        printf("1. Вывести всех студентов\n");
        printf("2. Поиск студента\n");
        printf("3. Сортировка студентов\n");
        printf("4. Записать в трейс-файл данные студента по ID\n");
        printf("5. Записать в трейс-файл студентов со средним баллом выше общего\n");
        printf("6. Выход\n");
        printf("Выберите пункт меню: ");

        char input_buf[64];
        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            break;
        }

        char *endptr = NULL;
        long menu_choice = strtol(input_buf, &endptr, 10);
        while (*endptr != '\0' && (*endptr == '\n' || *endptr == '\r' || isspace((unsigned char)*endptr))) {
            endptr++;
        }
        if (endptr == input_buf || *endptr != '\0') {
            fprintf(stderr, "Неверный ввод. Пожалуйста, введите число.\n");
            continue;
        }

        int choice = (int)menu_choice;

        switch (choice) {
            case 1: {
                print_students(stdout, &students);
                break;
            }
            case 2: {
                printf("Выберите критерий поиска:\n");
                printf("1. ID\n2. Фамилия\n3. Имя\n4. Группа\nВаш выбор: ");

                if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) break;
                long search_crit = strtol(input_buf, &endptr, 10);
                while (*endptr != '\0' && (*endptr == '\n' || *endptr == '\r' || isspace((unsigned char)*endptr))) {
                    endptr++;
                }
                if (endptr == input_buf || *endptr != '\0') {
                    fprintf(stderr, "Неверный выбор критерия.\n");
                    break;
                }
                int sc = (int)search_crit;

                student_ptr_vector_t found;
                if (ptr_vector_init(&found, 2) != STATUS_OK) {
                    fprintf(stderr, "Ошибка выделения памяти.\n");
                    break;
                }

                status_t find_st = STATUS_NOT_FOUND;

                if (sc == 1) {
                    printf("Введите ID: ");
                    if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
                        unsigned int id = 0;
                        if (parse_unsigned_int(input_buf, &id) == STATUS_OK) {
                            find_st = find_by_id(&students, id, &found);
                        } else {
                            fprintf(stderr, "Неверный формат ID.\n");
                        }
                    }
                } else if (sc == 2) {
                    printf("Введите фамилию: ");
                    if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
                        size_t len = strlen(input_buf);
                        while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) {
                            input_buf[len - 1] = '\0';
                            len--;
                        }
                        find_st = find_by_surname(&students, input_buf, &found);
                    }
                } else if (sc == 3) {
                    printf("Введите имя: ");
                    if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
                        size_t len = strlen(input_buf);
                        while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) {
                            input_buf[len - 1] = '\0';
                            len--;
                        }
                        find_st = find_by_name(&students, input_buf, &found);
                    }
                } else if (sc == 4) {
                    printf("Введите группу: ");
                    if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
                        size_t len = strlen(input_buf);
                        while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r')) {
                            input_buf[len - 1] = '\0';
                            len--;
                        }
                        find_st = find_by_group(&students, input_buf, &found);
                    }
                } else {
                    fprintf(stderr, "Неверный критерий поиска.\n");
                }

                if (find_st == STATUS_OK && found.size > 0) {
                    printf("Результаты поиска:\n");
                    print_ptr_students(stdout, &found);
                } else {
                    printf("Ничего не найдено или произошла ошибка.\n");
                }
                ptr_vector_free(&found);
                break;
            }
            case 3: {
                printf("Сортировать по:\n");
                printf("1. ID\n2. Фамилия\n3. Имя\n4. Группа\nВаш выбор: ");
                if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) break;
                long sort_crit = strtol(input_buf, &endptr, 10);
                while (*endptr != '\0' && (*endptr == '\n' || *endptr == '\r' || isspace((unsigned char)*endptr))) {
                    endptr++;
                }
                if (endptr == input_buf || *endptr != '\0') {
                    fprintf(stderr, "Неверный выбор сортировки.\n");
                    break;
                }
                int rc = (int)sort_crit;

                if (rc == 1) {
                    qsort(students.data, students.size, sizeof(student_t), compare_by_id);
                } else if (rc == 2) {
                    qsort(students.data, students.size, sizeof(student_t), compare_by_surname);
                } else if (rc == 3) {
                    qsort(students.data, students.size, sizeof(student_t), compare_by_name);
                } else if (rc == 4) {
                    qsort(students.data, students.size, sizeof(student_t), compare_by_group);
                } else {
                    fprintf(stderr, "Неверный критерий сортировки.\n");
                    break;
                }
                printf("Сортировка выполнена успешно.\n");
                print_students(stdout, &students);
                break;
            }
            case 4: {
                printf("Введите ID студента для трассировки: ");
                if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) break;
                unsigned int id = 0;
                if (parse_unsigned_int(input_buf, &id) != STATUS_OK) {
                    fprintf(stderr, "Неверный формат ID.\n");
                    break;
                }
                status_t st = trace_student_by_id(&students, id, trace_filename);
                if (st == STATUS_OK) {
                    printf("Данные успешно записаны в файл: %s\n", trace_filename);
                } else {
                    printf("Студент с ID %u не найден или ошибка записи.\n", id);
                }
                break;
            }
            case 5: {
                status_t st = trace_above_average_students(&students, trace_filename);
                if (st == STATUS_OK) {
                    printf("Список студентов выше среднего балла записан в файл: %s\n", trace_filename);
                } else {
                    printf("Ошибка записи в файл.\n");
                }
                break;
            }
            case 6: {
                running = false;
                break;
            }
            default:
                printf("Неверный выбор пункта меню.\n");
        }
    }

    vector_free(&students);
    return 0;
}