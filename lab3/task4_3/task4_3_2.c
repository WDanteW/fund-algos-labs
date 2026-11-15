#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "post.h"
#include "status.h"

#define LINE_CAP 256
#define WEIGHT_EPSILON 1e-9

typedef struct
{
    char city[LINE_CAP];
    char street[LINE_CAP];
    char house[LINE_CAP];
    char building[LINE_CAP];
    char apartment[LINE_CAP];
    char index[LINE_CAP];
} address_form_t;

typedef struct
{
    address_form_t address;
    char mail_id[LINE_CAP];
    char weight[LINE_CAP];
    char created_at[LINE_CAP];
    char delivery_time[LINE_CAP];
} mail_form_t;

static void trim_in_place(char *text)
{
    size_t length = strlen(text);
    size_t start = 0;
    while (text[start] != '\0' && isspace((unsigned char)text[start]))
    {
        start++;
    }
    while (length > start && isspace((unsigned char)text[length - 1]))
    {
        length--;
    }
    size_t new_length = length - start;
    memmove(text, text + start, new_length);
    text[new_length] = '\0';
}

static status_t read_line(char *buffer, size_t capacity)
{
    if (buffer == NULL || capacity < 2)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (fgets(buffer, (int)capacity, stdin) == NULL)
    {
        return STATUS_INPUT_ERROR;
    }
    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n')
    {
        buffer[length - 1] = '\0';
        return STATUS_OK;
    }
    int next = getchar();
    if (next == EOF || next == '\n')
    {
        return STATUS_OK;
    }
    while (next != EOF && next != '\n')
    {
        next = getchar();
    }
    return STATUS_LINE_TOO_LONG;
}

static status_t prompt_line(const char *prompt, char *buffer, size_t capacity)
{
    printf("%s", prompt);
    fflush(stdout);
    status_t status = read_line(buffer, capacity);
    if (status == STATUS_OK)
    {
        trim_in_place(buffer);
    }
    return status;
}

static status_t parse_natural(const char *text, int *value)
{
    if (text == NULL || value == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (text[0] == '\0')
    {
        return STATUS_INVALID_ARGUMENT;
    }
    int result = 0;
    for (size_t i = 0; text[i] != '\0'; i++)
    {
        if (text[i] < '0' || text[i] > '9')
        {
            return STATUS_INVALID_ARGUMENT;
        }
        int digit = text[i] - '0';
        if (result > (INT_MAX - digit) / 10)
        {
            return STATUS_OVERFLOW;
        }
        result = result * 10 + digit;
    }
    if (result == 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    *value = result;
    return STATUS_OK;
}

static status_t parse_weight(const char *text, double *value)
{
    if (text == NULL || value == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    size_t length = strlen(text);
    if (length == 0 || length >= LINE_CAP)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    char normalized[LINE_CAP];
    bool has_digit = false;
    bool has_point = false;
    for (size_t i = 0; i < length; i++)
    {
        char symbol = text[i] == ',' ? '.' : text[i];
        if (symbol >= '0' && symbol <= '9')
        {
            has_digit = true;
        }
        else if (symbol == '.' && !has_point)
        {
            has_point = true;
        }
        else
        {
            return STATUS_INVALID_ARGUMENT;
        }
        normalized[i] = symbol;
    }
    normalized[length] = '\0';
    if (!has_digit)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    char *end = NULL;
    double parsed = strtod(normalized, &end);
    if (end == normalized || *end != '\0' || !isfinite(parsed))
    {
        return STATUS_INVALID_ARGUMENT;
    }
    *value = parsed;
    return STATUS_OK;
}

static int parse_menu_choice(const char *text)
{
    if (strlen(text) == 1 && text[0] >= '0' && text[0] <= '7')
    {
        return text[0] - '0';
    }
    return -1;
}

static status_t read_address_form(address_form_t *form)
{
    status_t status = prompt_line("Город: ", form->city, LINE_CAP);
    if (status == STATUS_OK)
    {
        status = prompt_line("Улица: ", form->street, LINE_CAP);
    }
    if (status == STATUS_OK)
    {
        status = prompt_line("Номер дома: ", form->house, LINE_CAP);
    }
    if (status == STATUS_OK)
    {
        status = prompt_line("Корпус (можно оставить пустым): ", form->building,
                             LINE_CAP);
    }
    if (status == STATUS_OK)
    {
        status = prompt_line("Номер квартиры: ", form->apartment, LINE_CAP);
    }
    if (status == STATUS_OK)
    {
        status = prompt_line("Индекс (6 цифр): ", form->index, LINE_CAP);
    }
    return status;
}

static status_t read_mail_form(mail_form_t *form)
{
    status_t status =
        prompt_line("Идентификатор отправления (14 цифр): ", form->mail_id, LINE_CAP);
    if (status == STATUS_OK)
    {
        status = prompt_line("Вес: ", form->weight, LINE_CAP);
    }
    if (status == STATUS_OK)
    {
        printf("Адрес получателя\n");
        status = read_address_form(&form->address);
    }
    if (status == STATUS_OK)
    {
        status = prompt_line("Время создания (dd:MM:yyyy hh:mm:ss): ",
                             form->created_at, LINE_CAP);
    }
    if (status == STATUS_OK)
    {
        status = prompt_line("Срок доставки (dd:MM:yyyy hh:mm:ss): ",
                             form->delivery_time, LINE_CAP);
    }
    return status;
}

static status_t build_address(const address_form_t *form, Address *result)
{
    int house = 0;
    int apartment = 0;
    status_t status = parse_natural(form->house, &house);
    if (status == STATUS_OK)
    {
        status = parse_natural(form->apartment, &apartment);
    }
    if (status != STATUS_OK)
    {
        return status == STATUS_OVERFLOW ? status : STATUS_INVALID_ARGUMENT;
    }
    return address_create(result, form->city, form->street, house,
                          form->building, apartment, form->index);
}

static status_t build_mail(const mail_form_t *form, Mail *result)
{
    double weight = 0.0;
    status_t status = parse_weight(form->weight, &weight);
    if (status != STATUS_OK)
    {
        return status;
    }
    Address recipient;
    status = build_address(&form->address, &recipient);
    if (status != STATUS_OK)
    {
        return status;
    }
    status = mail_create(result, &recipient, weight, form->mail_id,
                         form->created_at, form->delivery_time, WEIGHT_EPSILON);
    address_destroy(&recipient);
    return status;
}

static void print_address(const Address *address)
{
    printf("г. %s, ул. %s, д. %d", address->city.data, address->street.data,
           address->house);
    if (address->building.length > 0)
    {
        printf(", корп. %s", address->building.data);
    }
    printf(", кв. %d, индекс %s", address->apartment, address->index.data);
}

static void format_key(uint64_t key, char *buffer, size_t capacity)
{
    unsigned year = (unsigned)(key / 10000000000ull);
    unsigned month = (unsigned)((key / 100000000ull) % 100u);
    unsigned day = (unsigned)((key / 1000000ull) % 100u);
    unsigned hour = (unsigned)((key / 10000ull) % 100u);
    unsigned minute = (unsigned)((key / 100ull) % 100u);
    unsigned second = (unsigned)(key % 100u);
    snprintf(buffer, capacity, "%02u:%02u:%04u %02u:%02u:%02u", day, month, year,
             hour, minute, second);
}

static void print_mail(const Mail *mail, uint64_t now_key)
{
    printf("  Идентификатор: %s\n", mail->mail_id.data);
    printf("  Получатель: ");
    print_address(&mail->recipient);
    printf("\n");
    printf("  Вес: %.3f\n", mail->weight);
    printf("  Создано: %s\n", mail->created_at.data);
    printf("  Срок доставки: %s\n", mail->delivery_time.data);
    if (mail->delivered)
    {
        printf("  Статус: доставлено\n");
    }
    else if (mail->delivery_key < now_key)
    {
        printf("  Статус: не доставлено, срок доставки истёк\n");
    }
    else
    {
        printf("  Статус: не доставлено, срок не истёк\n");
    }
}

static void print_menu(void)
{
    printf("\n=== Почтовое отделение ===\n");
    printf("1. Добавить отправление\n");
    printf("2. Удалить отправление\n");
    printf("3. Найти отправление по идентификатору\n");
    printf("4. Показать все отправления\n");
    printf("5. Показать доставленные отправления\n");
    printf("6. Показать отправления с истёкшим сроком доставки\n");
    printf("7. Отметить отправление врученным\n");
    printf("0. Выход\n");
}

static status_t setup_post(Post *post)
{
    printf("Введите адрес почтового отделения\n");
    for (;;)
    {
        address_form_t form;
        status_t status = read_address_form(&form);
        if (status == STATUS_INPUT_ERROR)
        {
            return status;
        }
        if (status == STATUS_OK)
        {
            Address address;
            status = build_address(&form, &address);
            if (status == STATUS_OK)
            {
                status = post_init(post, &address);
                address_destroy(&address);
                return status;
            }
        }
        printf("Ошибка: %s. Введите адрес заново.\n", status_message(status));
    }
}

static status_t handle_add(Post *post)
{
    mail_form_t form;
    status_t status = read_mail_form(&form);
    if (status != STATUS_OK)
    {
        return status;
    }
    Mail mail;
    status = build_mail(&form, &mail);
    if (status != STATUS_OK)
    {
        return status;
    }
    status = post_add_mail(post, &mail);
    if (status != STATUS_OK)
    {
        mail_destroy(&mail);
        return status;
    }
    printf("Отправление добавлено\n");
    return STATUS_OK;
}

static status_t handle_remove(Post *post)
{
    char id[LINE_CAP];
    status_t status = prompt_line("Идентификатор отправления: ", id, LINE_CAP);
    if (status != STATUS_OK)
    {
        return status;
    }
    status = post_remove_mail(post, id);
    if (status == STATUS_OK)
    {
        printf("Отправление удалено\n");
    }
    return status;
}

static status_t handle_find(const Post *post)
{
    char id[LINE_CAP];
    status_t status = prompt_line("Идентификатор отправления: ", id, LINE_CAP);
    if (status != STATUS_OK)
    {
        return status;
    }
    const Mail *mail = NULL;
    status = post_find_mail(post, id, &mail);
    if (status != STATUS_OK)
    {
        return status;
    }
    uint64_t now_key = 0;
    status = datetime_now_key(&now_key);
    if (status != STATUS_OK)
    {
        return status;
    }
    printf("Информация об отправлении:\n");
    print_mail(mail, now_key);
    return STATUS_OK;
}

static status_t handle_mark(Post *post)
{
    char id[LINE_CAP];
    status_t status = prompt_line("Идентификатор отправления: ", id, LINE_CAP);
    if (status != STATUS_OK)
    {
        return status;
    }
    status = post_mark_delivered(post, id);
    if (status == STATUS_OK)
    {
        printf("Отправление отмечено как врученное\n");
    }
    return status;
}

static status_t handle_list_all(const Post *post)
{
    uint64_t now_key = 0;
    status_t status = datetime_now_key(&now_key);
    if (status != STATUS_OK)
    {
        return status;
    }
    if (post->count == 0)
    {
        printf("В отделении нет отправлений\n");
        return STATUS_OK;
    }
    printf("Всего отправлений: %zu (по индексу получателя и идентификатору)\n",
           post->count);
    for (size_t i = 0; i < post->count; i++)
    {
        printf("%zu.\n", i + 1);
        print_mail(&post->mails[i], now_key);
    }
    return STATUS_OK;
}

static status_t handle_list_filtered(const Post *post, mail_filter_t filter)
{
    uint64_t now_key = 0;
    status_t status = datetime_now_key(&now_key);
    if (status != STATUS_OK)
    {
        return status;
    }
    char now_text[LINE_CAP];
    format_key(now_key, now_text, sizeof(now_text));
    printf("Текущее системное время: %s\n", now_text);

    const Mail **items = NULL;
    size_t count = 0;
    status = post_select_mails(post, filter, now_key, &items, &count);
    if (status != STATUS_OK)
    {
        return status;
    }
    if (count == 0)
    {
        printf("Подходящих отправлений нет\n");
    }
    else
    {
        printf("Найдено: %zu (от старых к новым по времени создания)\n", count);
        for (size_t i = 0; i < count; i++)
        {
            printf("%zu.\n", i + 1);
            print_mail(items[i], now_key);
        }
    }
    free(items);
    return STATUS_OK;
}

static status_t run_dialog(Post *post)
{
    for (;;)
    {
        print_menu();
        char line[LINE_CAP];
        status_t status = prompt_line("Выберите пункт: ", line, LINE_CAP);
        if (status == STATUS_INPUT_ERROR)
        {
            return STATUS_OK;
        }
        if (status != STATUS_OK)
        {
            printf("Ошибка: %s\n", status_message(status));
            continue;
        }

        int choice = parse_menu_choice(line);
        if (choice == 0)
        {
            return STATUS_OK;
        }
        switch (choice)
        {
        case 1:
            status = handle_add(post);
            break;
        case 2:
            status = handle_remove(post);
            break;
        case 3:
            status = handle_find(post);
            break;
        case 4:
            status = handle_list_all(post);
            break;
        case 5:
            status = handle_list_filtered(post, MAIL_FILTER_DELIVERED);
            break;
        case 6:
            status = handle_list_filtered(post, MAIL_FILTER_EXPIRED);
            break;
        case 7:
            status = handle_mark(post);
            break;
        default:
            printf("Неизвестный пункт меню\n");
            continue;
        }
        if (status == STATUS_INPUT_ERROR)
        {
            return STATUS_OK;
        }
        if (status != STATUS_OK)
        {
            printf("Ошибка: %s\n", status_message(status));
        }
    }
}

int main(void)
{
    Post post = {NULL, NULL, 0, 0};
    status_t status = setup_post(&post);
    if (status != STATUS_OK)
    {
        if (status != STATUS_INPUT_ERROR)
        {
            fprintf(stderr, "Ошибка: %s\n", status_message(status));
            return 1;
        }
        return 0;
    }

    status = run_dialog(&post);
    post_destroy(&post);
    if (status != STATUS_OK)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(status));
        return 1;
    }
    return 0;
}