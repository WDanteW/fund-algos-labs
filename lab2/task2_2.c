#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    unsigned int id;
    char name[64];
    char surname[64];
    double salary;
} Employee;

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_ARG_COUNT,
    ERROR_INVALID_FLAG,
    ERROR_FILE_OPEN,
    ERROR_MEMORY_ALLOCATION,
    ERROR_INVALID_FILE_FORMAT
} status_code;

typedef enum {
    SORT_ASCENDING = 1,
    SORT_DESCENDING = 2
} sort_order;

static int compare_employees_asc(const void *a, const void *b) {
    const Employee *emp1 = (const Employee *)a;
    const Employee *emp2 = (const Employee *)b;

    if (emp1->salary < emp2->salary) return -1;
    if (emp1->salary > emp2->salary) return 1;

    int surname_cmp = strcmp(emp1->surname, emp2->surname);
    if (surname_cmp != 0) return surname_cmp;

    int name_cmp = strcmp(emp1->name, emp2->name);
    if (name_cmp != 0) return name_cmp;

    if (emp1->id < emp2->id) return -1;
    if (emp1->id > emp2->id) return 1;

    return 0;
}

static int compare_employees_desc(const void *a, const void *b) {
    const Employee *emp1 = (const Employee *)a;
    const Employee *emp2 = (const Employee *)b;

    if (emp1->salary > emp2->salary) return -1;
    if (emp1->salary < emp2->salary) return 1;

    int surname_cmp = strcmp(emp2->surname, emp1->surname);
    if (surname_cmp != 0) return surname_cmp;

    int name_cmp = strcmp(emp2->name, emp1->name);
    if (name_cmp != 0) return name_cmp;

    if (emp1->id > emp2->id) return -1;
    if (emp1->id < emp2->id) return 1;

    return 0;
}

static status_code read_employees(const char *filename, Employee **employees, size_t *count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return ERROR_FILE_OPEN;
    }

    size_t capacity = 10;
    size_t size = 0;
    Employee *arr = (Employee *)malloc(capacity * sizeof(Employee));
    if (arr == NULL) {
        fclose(file);
        return ERROR_MEMORY_ALLOCATION;
    }

    char id_str[64];
    char name[64];
    char surname[64];
    char salary_str[64];

    while (fscanf(file, "%63s %63s %63s %63s", id_str, name, surname, salary_str) == 4) {
        char *endptr_id = NULL;
        char *endptr_sal = NULL;

        unsigned long parsed_id = strtoul(id_str, &endptr_id, 10);
        double parsed_salary = strtod(salary_str, &endptr_sal);

        if (*endptr_id != '\0' || parsed_id > UINT_MAX || *endptr_sal != '\0' || parsed_salary < 0.0) {
            free(arr);
            fclose(file);
            return ERROR_INVALID_FILE_FORMAT;
        }

        if (size >= capacity) {
            capacity *= 2;
            Employee *temp = (Employee *)realloc(arr, capacity * sizeof(Employee));
            if (temp == NULL) {
                free(arr);
                fclose(file);
                return ERROR_MEMORY_ALLOCATION;
            }
            arr = temp;
        }

        arr[size].id = (unsigned int)parsed_id;
        for (size_t i = 0; i <= strlen(name); ++i) {
            arr[size].name[i] = name[i];
        }
        for (size_t i = 0; i <= strlen(surname); ++i) {
            arr[size].surname[i] = surname[i];
        }
        arr[size].salary = parsed_salary;
        size++;
    }

    if (!feof(file)) {
        free(arr);
        fclose(file);
        return ERROR_INVALID_FILE_FORMAT;
    }

    fclose(file);
    *employees = arr;
    *count = size;
    return SUCCESS;
}

static status_code write_employees(const char *filename, const Employee *employees, size_t count) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return ERROR_FILE_OPEN;
    }

    for (size_t i = 0; i < count; ++i) {
        if (fprintf(file, "%u %s %s %.2f\n", employees[i].id, employees[i].name, employees[i].surname, employees[i].salary) < 0) {
            fclose(file);
            return ERROR_FILE_OPEN;
        }
    }

    fclose(file);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Ошибка: неверное количество аргументов.\n");
        return ERROR_INVALID_ARG_COUNT;
    }

    const char *input_filename = argv[1];
    const char *flag = argv[2];
    const char *output_filename = argv[3];

    if ((flag[0] != '-' && flag[0] != '/') || flag[1] == '\0' || flag[2] != '\0') {
        fprintf(stderr, "Ошибка: неверный формат флага.\n");
        return ERROR_INVALID_FLAG;
    }

    sort_order order;
    if (flag[1] == 'a') {
        order = SORT_ASCENDING;
    } else if (flag[1] == 'd') {
        order = SORT_DESCENDING;
    } else {
        fprintf(stderr, "Ошибка: неизвестный флаг.\n");
        return ERROR_INVALID_FLAG;
    }

    Employee *employees = NULL;
    size_t count = 0;

    status_code status = read_employees(input_filename, &employees, &count);
    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка при чтении файла.\n");
        return status;
    }

    if (count > 0) {
        if (order == SORT_ASCENDING) {
            qsort(employees, count, sizeof(Employee), compare_employees_asc);
        } else {
            qsort(employees, count, sizeof(Employee), compare_employees_desc);
        }
    }

    status = write_employees(output_filename, employees, count);
    free(employees);

    if (status != SUCCESS) {
        fprintf(stderr, "Ошибка при записи в выходной файл.\n");
        return status;
    }

    return SUCCESS;
}