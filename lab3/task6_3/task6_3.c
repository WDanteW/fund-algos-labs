#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#define EPSILON 1e-9

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_NO_MEMORY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_NOT_FOUND
} status_t;

typedef enum {
    STOP_INITIAL = 0,
    STOP_INTERMEDIATE,
    STOP_FINAL
} stop_type_t;

typedef struct stop_node {
    double x;
    double y;
    long long arrival_sec;
    long long departure_sec;
    char arrival_str[64];
    char departure_str[64];
    stop_type_t type;
    struct stop_node *next;
} stop_node_t;

typedef struct route_node {
    stop_node_t *stops;
    double total_distance;
    long long total_idle_sec;
    struct route_node *next;
} route_node_t;

typedef struct vehicle_node {
    char *vehicle_id;
    stop_node_t *raw_stops;
    route_node_t *routes;
    size_t route_count;
    double total_distance;
    double max_route_distance;
    double min_route_distance;
    long long max_single_stop_sec;
    long long min_single_stop_sec;
    long long total_idle_sec;
    struct vehicle_node *next;
} vehicle_node_t;

typedef enum {
    QUERY_MAX_ROUTES = 1,
    QUERY_MIN_ROUTES,
    QUERY_MAX_TOTAL_DIST,
    QUERY_MIN_TOTAL_DIST,
    QUERY_MAX_SINGLE_ROUTE,
    QUERY_MIN_SINGLE_ROUTE,
    QUERY_MAX_SINGLE_STOP,
    QUERY_MIN_SINGLE_STOP,
    QUERY_MAX_TOTAL_IDLE
} query_type_t;

static char *my_strdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char *)malloc(len);
    if (p == NULL) return NULL;
    memcpy(p, s, len);
    return p;
}

static bool real_greater(double left, double right, double epsilon) {
    return (left - right) > epsilon;
}

static bool real_less(double left, double right, double epsilon) {
    return (right - left) > epsilon;
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

static status_t parse_two_digits(const char *s, int *out) {
    if (s == NULL || out == NULL) return STATUS_NULL_POINTER;
    if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1])) return STATUS_INVALID_INPUT;
    *out = (s[0] - '0') * 10 + (s[1] - '0');
    return STATUS_OK;
}

static status_t parse_four_digits(const char *s, int *out) {
    if (s == NULL || out == NULL) return STATUS_NULL_POINTER;
    for (int i = 0; i < 4; ++i) {
        if (!isdigit((unsigned char)s[i])) return STATUS_INVALID_INPUT;
    }
    *out = (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0');
    return STATUS_OK;
}

static status_t parse_datetime(const char *date_str, const char *time_str, long long *out_seconds) {
    if (date_str == NULL || time_str == NULL || out_seconds == NULL) return STATUS_NULL_POINTER;
    if (strlen(date_str) != 10 || strlen(time_str) != 8) return STATUS_INVALID_INPUT;
    if (date_str[2] != '.' || date_str[5] != '.') return STATUS_INVALID_INPUT;
    if (time_str[2] != ':' || time_str[5] != ':') return STATUS_INVALID_INPUT;

    int day = 0, month = 0, year = 0;
    int hour = 0, minute = 0, second = 0;

    if (parse_two_digits(date_str, &day) != STATUS_OK ||
        parse_two_digits(date_str + 3, &month) != STATUS_OK ||
        parse_four_digits(date_str + 6, &year) != STATUS_OK ||
        parse_two_digits(time_str, &hour) != STATUS_OK ||
        parse_two_digits(time_str + 3, &minute) != STATUS_OK ||
        parse_two_digits(time_str + 6, &second) != STATUS_OK) {
        return STATUS_INVALID_INPUT;
    }

    if (year < 1970 || year > 9999) return STATUS_INVALID_INPUT;
    if (month < 1 || month > 12) return STATUS_INVALID_INPUT;
    int max_day = days_in_month(month, year);
    if (day < 1 || day > max_day) return STATUS_INVALID_INPUT;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return STATUS_INVALID_INPUT;
    }

    long long total_days = 0;
    for (int y = 1970; y < year; ++y) {
        total_days += is_leap_year(y) ? 366 : 365;
    }
    for (int m = 1; m < month; ++m) {
        total_days += days_in_month(m, year);
    }
    total_days += (day - 1);

    *out_seconds = total_days * 86400LL + (long long)hour * 3600LL + (long long)minute * 60LL + (long long)second;
    return STATUS_OK;
}

static status_t parse_double_strict(const char *str, double *out) {
    if (str == NULL || out == NULL || *str == '\0') return STATUS_INVALID_INPUT;
    char *endptr = NULL;
    double val = strtod(str, &endptr);
    if (endptr == str || *endptr != '\0' || !isfinite(val)) return STATUS_INVALID_INPUT;
    *out = val;
    return STATUS_OK;
}

static status_t parse_marker(const char *str, stop_type_t *out) {
    if (str == NULL || out == NULL) return STATUS_NULL_POINTER;
    if (strcmp(str, "INITIAL") == 0) {
        *out = STOP_INITIAL;
        return STATUS_OK;
    }
    if (strcmp(str, "INTERMEDIATE") == 0) {
        *out = STOP_INTERMEDIATE;
        return STATUS_OK;
    }
    if (strcmp(str, "FINAL") == 0) {
        *out = STOP_FINAL;
        return STATUS_OK;
    }
    return STATUS_INVALID_INPUT;
}

static stop_node_t *create_stop_node(double x, double y, long long arr_sec, long long dep_sec,
                                     const char *arr_str, const char *dep_str, stop_type_t type) {
    stop_node_t *node = (stop_node_t *)malloc(sizeof(stop_node_t));
    if (node == NULL) return NULL;
    node->x = x;
    node->y = y;
    node->arrival_sec = arr_sec;
    node->departure_sec = dep_sec;
    strncpy(node->arrival_str, arr_str, sizeof(node->arrival_str) - 1);
    node->arrival_str[sizeof(node->arrival_str) - 1] = '\0';
    strncpy(node->departure_str, dep_str, sizeof(node->departure_str) - 1);
    node->departure_str[sizeof(node->departure_str) - 1] = '\0';
    node->type = type;
    node->next = NULL;
    return node;
}

static void insert_stop_sorted(stop_node_t **head, stop_node_t *new_node) {
    if (head == NULL || new_node == NULL) return;
    if (*head == NULL || new_node->arrival_sec < (*head)->arrival_sec ||
        (new_node->arrival_sec == (*head)->arrival_sec && new_node->departure_sec < (*head)->departure_sec)) {
        new_node->next = *head;
        *head = new_node;
        return;
    }
    stop_node_t *curr = *head;
    while (curr->next != NULL &&
           (curr->next->arrival_sec < new_node->arrival_sec ||
            (curr->next->arrival_sec == new_node->arrival_sec && curr->next->departure_sec <= new_node->departure_sec))) {
        curr = curr->next;
    }
    new_node->next = curr->next;
    curr->next = new_node;
}

static void free_stop_list(stop_node_t *head) {
    while (head != NULL) {
        stop_node_t *tmp = head;
        head = head->next;
        free(tmp);
    }
}

static void free_route_list(route_node_t *head) {
    while (head != NULL) {
        route_node_t *tmp = head;
        head = head->next;
        free_stop_list(tmp->stops);
        free(tmp);
    }
}

static void free_vehicle_list(vehicle_node_t *head) {
    while (head != NULL) {
        vehicle_node_t *tmp = head;
        head = head->next;
        free(tmp->vehicle_id);
        free_stop_list(tmp->raw_stops);
        free_route_list(tmp->routes);
        free(tmp);
    }
}

static status_t get_or_create_vehicle(vehicle_node_t **head, const char *vehicle_id, vehicle_node_t **out_vehicle) {
    if (head == NULL || vehicle_id == NULL || out_vehicle == NULL) return STATUS_NULL_POINTER;

    vehicle_node_t *curr = *head;
    while (curr != NULL) {
        if (strcmp(curr->vehicle_id, vehicle_id) == 0) {
            *out_vehicle = curr;
            return STATUS_OK;
        }
        curr = curr->next;
    }

    vehicle_node_t *new_v = (vehicle_node_t *)malloc(sizeof(vehicle_node_t));
    if (new_v == NULL) return STATUS_NO_MEMORY;

    new_v->vehicle_id = my_strdup(vehicle_id);
    if (new_v->vehicle_id == NULL) {
        free(new_v);
        return STATUS_NO_MEMORY;
    }

    new_v->raw_stops = NULL;
    new_v->routes = NULL;
    new_v->route_count = 0;
    new_v->total_distance = 0.0;
    new_v->max_route_distance = 0.0;
    new_v->min_route_distance = 0.0;
    new_v->max_single_stop_sec = 0;
    new_v->min_single_stop_sec = 0;
    new_v->total_idle_sec = 0;
    new_v->next = *head;
    *head = new_v;

    *out_vehicle = new_v;
    return STATUS_OK;
}

static status_t process_stop_file(const char *filepath, vehicle_node_t **vehicles) {
    if (filepath == NULL || vehicles == NULL) return STATUS_NULL_POINTER;

    FILE *file = fopen(filepath, "r");
    if (file == NULL) return STATUS_FILE_OPEN_ERROR;

    char line[512];
    bool coords_parsed = false;
    double stop_x = 0.0;
    double stop_y = 0.0;
    status_t status = STATUS_OK;

    while (status == STATUS_OK && fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || isspace((unsigned char)line[len - 1]))) {
            line[len - 1] = '\0';
            len--;
        }
        if (len == 0) continue;

        if (!coords_parsed) {
            char x_str[64], y_str[64], extra[8];
            if (sscanf(line, "%63s %63s %7s", x_str, y_str, extra) != 2 ||
                parse_double_strict(x_str, &stop_x) != STATUS_OK ||
                parse_double_strict(y_str, &stop_y) != STATUS_OK) {
                status = STATUS_INVALID_INPUT;
                break;
            }
            coords_parsed = true;
            continue;
        }

        char veh_id[128], arr_d[32], arr_t[32], dep_d[32], dep_t[32], marker_str[32], extra[8];
        if (sscanf(line, "%127s %31s %31s %31s %31s %31s %7s",
                   veh_id, arr_d, arr_t, dep_d, dep_t, marker_str, extra) != 6) {
            status = STATUS_INVALID_INPUT;
            break;
        }

        long long arr_sec = 0;
        long long dep_sec = 0;
        stop_type_t marker = STOP_INTERMEDIATE;

        if (parse_datetime(arr_d, arr_t, &arr_sec) != STATUS_OK ||
            parse_datetime(dep_d, dep_t, &dep_sec) != STATUS_OK ||
            dep_sec < arr_sec ||
            parse_marker(marker_str, &marker) != STATUS_OK) {
            status = STATUS_INVALID_INPUT;
            break;
        }

        char arr_full[64];
        char dep_full[64];
        snprintf(arr_full, sizeof(arr_full), "%s %s", arr_d, arr_t);
        snprintf(dep_full, sizeof(dep_full), "%s %s", dep_d, dep_t);

        vehicle_node_t *veh = NULL;
        status = get_or_create_vehicle(vehicles, veh_id, &veh);
        if (status != STATUS_OK) break;

        stop_node_t *node = create_stop_node(stop_x, stop_y, arr_sec, dep_sec, arr_full, dep_full, marker);
        if (node == NULL) {
            status = STATUS_NO_MEMORY;
            break;
        }

        insert_stop_sorted(&veh->raw_stops, node);
    }

    if (status == STATUS_OK && ferror(file)) {
        status = STATUS_FILE_READ_ERROR;
    }
    if (status == STATUS_OK && !coords_parsed) {
        status = STATUS_INVALID_INPUT;
    }

    fclose(file);
    return status;
}

static status_t build_routes_for_vehicles(vehicle_node_t *vehicles) {
    vehicle_node_t *v = vehicles;
    while (v != NULL) {
        stop_node_t *curr = v->raw_stops;
        route_node_t *current_route = NULL;
        stop_node_t *route_tail = NULL;
        route_node_t *routes_tail = v->routes;
        if (routes_tail != NULL) {
            while (routes_tail->next != NULL) {
                routes_tail = routes_tail->next;
            }
        }

        bool has_any_stop = false;
        v->max_single_stop_sec = 0;
        v->min_single_stop_sec = LLONG_MAX;
        v->total_idle_sec = 0;

        while (curr != NULL) {
            stop_node_t *next_raw = curr->next;
            curr->next = NULL;

            if (curr->type == STOP_INITIAL) {
                if (current_route != NULL) {
                    free_stop_list(current_route->stops);
                    free(current_route);
                }
                current_route = (route_node_t *)malloc(sizeof(route_node_t));
                if (current_route == NULL) {
                    free(curr);
                    free_stop_list(next_raw);
                    v->raw_stops = NULL;
                    return STATUS_NO_MEMORY;
                }
                current_route->stops = curr;
                current_route->total_distance = 0.0;
                current_route->total_idle_sec = 0;
                current_route->next = NULL;
                route_tail = curr;
            } else {
                if (current_route != NULL) {
                    double dx = curr->x - route_tail->x;
                    double dy = curr->y - route_tail->y;
                    double dist = sqrt(dx * dx + dy * dy);
                    if (!isfinite(dist)) {
                        free(curr);
                        free_stop_list(next_raw);
                        free_stop_list(current_route->stops);
                        free(current_route);
                        v->raw_stops = NULL;
                        return STATUS_INVALID_INPUT;
                    }
                    current_route->total_distance += dist;
                    route_tail->next = curr;
                    route_tail = curr;

                    if (curr->type == STOP_FINAL) {
                        stop_node_t *route_curr = current_route->stops;
                        while (route_curr != NULL) {
                            long long idle = route_curr->departure_sec - route_curr->arrival_sec;
                            current_route->total_idle_sec += idle;
                            v->total_idle_sec += idle;
                            if (!has_any_stop) {
                                v->max_single_stop_sec = idle;
                                v->min_single_stop_sec = idle;
                                has_any_stop = true;
                            } else {
                                if (idle > v->max_single_stop_sec) v->max_single_stop_sec = idle;
                                if (idle < v->min_single_stop_sec) v->min_single_stop_sec = idle;
                            }
                            route_curr = route_curr->next;
                        }

                        if (v->routes == NULL) {
                            v->routes = current_route;
                        } else {
                            routes_tail->next = current_route;
                        }
                        routes_tail = current_route;

                        if (v->route_count == 0) {
                            v->max_route_distance = current_route->total_distance;
                            v->min_route_distance = current_route->total_distance;
                        } else {
                            if (real_greater(current_route->total_distance, v->max_route_distance, EPSILON)) {
                                v->max_route_distance = current_route->total_distance;
                            }
                            if (real_less(current_route->total_distance, v->min_route_distance, EPSILON)) {
                                v->min_route_distance = current_route->total_distance;
                            }
                        }

                        v->total_distance += current_route->total_distance;
                        v->route_count++;

                        current_route = NULL;
                        route_tail = NULL;
                    }
                } else {
                    free(curr);
                }
            }

            curr = next_raw;
        }

        if (current_route != NULL) {
            free_stop_list(current_route->stops);
            free(current_route);
        }

        v->raw_stops = NULL;
        if (!has_any_stop) {
            v->min_single_stop_sec = 0;
        }

        v = v->next;
    }

    return STATUS_OK;
}

static status_t find_vehicle_by_query(const vehicle_node_t *vehicles, query_type_t query,
                                      double epsilon, const vehicle_node_t **result) {
    if (vehicles == NULL || result == NULL) return STATUS_NULL_POINTER;

    const vehicle_node_t *best = NULL;
    const vehicle_node_t *curr = vehicles;

    while (curr != NULL) {
        bool consider = true;
        if ((query == QUERY_MAX_SINGLE_ROUTE || query == QUERY_MIN_SINGLE_ROUTE) && curr->route_count == 0) {
            consider = false;
        }

        if (consider) {
            if (best == NULL) {
                best = curr;
            } else {
                bool is_better = false;
                switch (query) {
                    case QUERY_MAX_ROUTES:
                        is_better = (curr->route_count > best->route_count);
                        break;
                    case QUERY_MIN_ROUTES:
                        is_better = (curr->route_count < best->route_count);
                        break;
                    case QUERY_MAX_TOTAL_DIST:
                        is_better = real_greater(curr->total_distance, best->total_distance, epsilon);
                        break;
                    case QUERY_MIN_TOTAL_DIST:
                        is_better = real_less(curr->total_distance, best->total_distance, epsilon);
                        break;
                    case QUERY_MAX_SINGLE_ROUTE:
                        is_better = real_greater(curr->max_route_distance, best->max_route_distance, epsilon);
                        break;
                    case QUERY_MIN_SINGLE_ROUTE:
                        is_better = real_less(curr->min_route_distance, best->min_route_distance, epsilon);
                        break;
                    case QUERY_MAX_SINGLE_STOP:
                        is_better = (curr->max_single_stop_sec > best->max_single_stop_sec);
                        break;
                    case QUERY_MIN_SINGLE_STOP:
                        is_better = (curr->min_single_stop_sec < best->min_single_stop_sec);
                        break;
                    case QUERY_MAX_TOTAL_IDLE:
                        is_better = (curr->total_idle_sec > best->total_idle_sec);
                        break;
                }
                if (is_better) {
                    best = curr;
                }
            }
        }
        curr = curr->next;
    }

    if (best == NULL) return STATUS_NOT_FOUND;
    *result = best;
    return STATUS_OK;
}

static const char *stop_type_to_str(stop_type_t type) {
    switch (type) {
        case STOP_INITIAL: return "НАЧАЛЬНАЯ";
        case STOP_INTERMEDIATE: return "ПРОМЕЖУТОЧНАЯ";
        case STOP_FINAL: return "КОНЕЧНАЯ";
        default: return "НЕИЗВЕСТНО";
    }
}

static void print_all_storage(FILE *stream, const vehicle_node_t *vehicles) {
    if (stream == NULL) return;
    if (vehicles == NULL) {
        fprintf(stream, "Хранилище пусто.\n");
        return;
    }

    const vehicle_node_t *v = vehicles;
    while (v != NULL) {
        fprintf(stream, "\nТранспортное средство: %s\n", v->vehicle_id);
        fprintf(stream, "  Завершённых маршрутов: %zu | Суммарный путь: %.3f | Общий простой: %lld сек.\n",
                v->route_count, v->total_distance, v->total_idle_sec);
        const route_node_t *r = v->routes;
        size_t r_idx = 1;
        while (r != NULL) {
            fprintf(stream, "  Маршрут #%zu (длина: %.3f, простой: %lld сек.):\n",
                    r_idx++, r->total_distance, r->total_idle_sec);
            const stop_node_t *s = r->stops;
            while (s != NULL) {
                fprintf(stream, "    [%.2f; %.2f] Прибытие: %s | Отправление: %s | %s\n",
                        s->x, s->y, s->arrival_str, s->departure_str, stop_type_to_str(s->type));
                s = s->next;
            }
            r = r->next;
        }
        v = v->next;
    }
}

static void print_query_result(FILE *stream, query_type_t query, const vehicle_node_t *v) {
    if (stream == NULL || v == NULL) return;
    switch (query) {
        case QUERY_MAX_ROUTES:
            fprintf(stream, "ТС с наибольшим числом маршрутов: %s (маршрутов: %zu)\n", v->vehicle_id, v->route_count);
            break;
        case QUERY_MIN_ROUTES:
            fprintf(stream, "ТС с наименьшим числом маршрутов: %s (маршрутов: %zu)\n", v->vehicle_id, v->route_count);
            break;
        case QUERY_MAX_TOTAL_DIST:
            fprintf(stream, "ТС с самым длинным суммарным путём: %s (путь: %.3f)\n", v->vehicle_id, v->total_distance);
            break;
        case QUERY_MIN_TOTAL_DIST:
            fprintf(stream, "ТС с самым коротким суммарным путём: %s (путь: %.3f)\n", v->vehicle_id, v->total_distance);
            break;
        case QUERY_MAX_SINGLE_ROUTE:
            fprintf(stream, "ТС с самым длинным маршрутом: %s (длина маршрута: %.3f)\n", v->vehicle_id, v->max_route_distance);
            break;
        case QUERY_MIN_SINGLE_ROUTE:
            fprintf(stream, "ТС с самым коротким маршрутом: %s (длина маршрута: %.3f)\n", v->vehicle_id, v->min_route_distance);
            break;
        case QUERY_MAX_SINGLE_STOP:
            fprintf(stream, "ТС с самой долгой остановкой: %s (время остановки: %lld сек.)\n", v->vehicle_id, v->max_single_stop_sec);
            break;
        case QUERY_MIN_SINGLE_STOP:
            fprintf(stream, "ТС с самой короткой остановкой: %s (время остановки: %lld сек.)\n", v->vehicle_id, v->min_single_stop_sec);
            break;
        case QUERY_MAX_TOTAL_IDLE:
            fprintf(stream, "ТС с самым большим общим временем простоя: %s (простой: %lld сек.)\n", v->vehicle_id, v->total_idle_sec);
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <файл_остановки_1> [файл_остановки_2 ...]\n", argv[0]);
        return 1;
    }

    vehicle_node_t *vehicles = NULL;
    status_t status = STATUS_OK;

    for (int i = 1; i < argc; ++i) {
        status = process_stop_file(argv[i], &vehicles);
        if (status != STATUS_OK) {
            fprintf(stderr, "Ошибка при обработке файла '%s' (код: %d).\n", argv[i], status);
            free_vehicle_list(vehicles);
            return 1;
        }
    }

    status = build_routes_for_vehicles(vehicles);
    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка при формировании маршрутов (код: %d).\n", status);
        free_vehicle_list(vehicles);
        return 1;
    }

    if (vehicles == NULL) {
        printf("Записи о транспортных средствах не найдены.\n");
        return 0;
    }

    bool running = true;
    char input_buf[64];

    while (running) {
        printf("\n=== Система учёта городского транспорта ===\n");
        printf("1. ТС, проехавшее больше всех маршрутов\n");
        printf("2. ТС, проехавшее меньше всех маршрутов\n");
        printf("3. ТС, проехавшее самый длинный суммарный путь\n");
        printf("4. ТС, проехавшее самый короткий суммарный путь\n");
        printf("5. ТС с самым длинным отдельным маршрутом\n");
        printf("6. ТС с самым коротким отдельным маршрутом\n");
        printf("7. ТС с самой долгой остановкой\n");
        printf("8. ТС с самой короткой остановкой\n");
        printf("9. ТС с самым большим общим временем простоя\n");
        printf("10. Вывести все построенные маршруты\n");
        printf("0. Выход\n");
        printf("Выберите пункт меню: ");

        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            break;
        }

        char *endptr = NULL;
        long choice = strtol(input_buf, &endptr, 10);
        while (*endptr != '\0' && isspace((unsigned char)*endptr)) {
            endptr++;
        }

        if (endptr == input_buf || *endptr != '\0' || choice < 0 || choice > 10) {
            fprintf(stderr, "Неверный ввод. Введите число от 0 до 10.\n");
            continue;
        }

        if (choice == 0) {
            running = false;
        } else if (choice == 10) {
            print_all_storage(stdout, vehicles);
        } else {
            const vehicle_node_t *found = NULL;
            query_type_t q = (query_type_t)choice;
            status_t q_st = find_vehicle_by_query(vehicles, q, EPSILON, &found);
            if (q_st == STATUS_OK) {
                print_query_result(stdout, q, found);
            } else {
                printf("Подходящее транспортное средство не найдено.\n");
            }
        }
    }

    free_vehicle_list(vehicles);
    return 0;
}