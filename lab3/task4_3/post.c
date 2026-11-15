#include "post.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INITIAL_CAPACITY 4u

static bool is_digit_char(char symbol)
{
    return symbol >= '0' && symbol <= '9';
}

static bool is_digit_string(const char *text, size_t length)
{
    if (strlen(text) != length)
    {
        return false;
    }
    for (size_t i = 0; i < length; i++)
    {
        if (!is_digit_char(text[i]))
        {
            return false;
        }
    }
    return true;
}

static bool is_blank(const char *text)
{
    for (size_t i = 0; text[i] != '\0'; i++)
    {
        if (!isspace((unsigned char)text[i]))
        {
            return false;
        }
    }
    return true;
}

static bool real_less(double left, double right, double epsilon)
{
    return right - left > epsilon;
}

static bool is_leap_year(unsigned year)
{
    return (year % 4u == 0u && year % 100u != 0u) || year % 400u == 0u;
}

static unsigned days_in_month(unsigned year, unsigned month)
{
    static const unsigned days[] = {31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};
    if (month == 2u && is_leap_year(year))
    {
        return 29u;
    }
    return days[month - 1u];
}

static unsigned two_digits(const char *text)
{
    return (unsigned)((text[0] - '0') * 10 + (text[1] - '0'));
}

static uint64_t compose_key(unsigned year, unsigned month, unsigned day,
                            unsigned hour, unsigned minute, unsigned second)
{
    uint64_t key = year;
    key = key * 100u + month;
    key = key * 100u + day;
    key = key * 100u + hour;
    key = key * 100u + minute;
    key = key * 100u + second;
    return key;
}

status_t datetime_parse(const char *text, uint64_t *key)
{
    if (text == NULL || key == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (strlen(text) != DATETIME_LENGTH)
    {
        return STATUS_INVALID_DATETIME;
    }
    for (size_t i = 0; i < DATETIME_LENGTH; i++)
    {
        if (i == 2 || i == 5 || i == 13 || i == 16)
        {
            if (text[i] != ':')
            {
                return STATUS_INVALID_DATETIME;
            }
        }
        else if (i == 10)
        {
            if (text[i] != ' ')
            {
                return STATUS_INVALID_DATETIME;
            }
        }
        else if (!is_digit_char(text[i]))
        {
            return STATUS_INVALID_DATETIME;
        }
    }

    unsigned day = two_digits(text);
    unsigned month = two_digits(text + 3);
    unsigned year = two_digits(text + 6) * 100u + two_digits(text + 8);
    unsigned hour = two_digits(text + 11);
    unsigned minute = two_digits(text + 14);
    unsigned second = two_digits(text + 17);

    if (year < 1u || month < 1u || month > 12u || day < 1u ||
        day > days_in_month(year, month) || hour > 23u || minute > 59u ||
        second > 59u)
    {
        return STATUS_INVALID_DATETIME;
    }
    *key = compose_key(year, month, day, hour, minute, second);
    return STATUS_OK;
}

status_t datetime_now_key(uint64_t *key)
{
    if (key == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    time_t now = time(NULL);
    if (now == (time_t)-1)
    {
        return STATUS_SYSTEM_TIME_ERROR;
    }
    const struct tm *local = localtime(&now);
    if (local == NULL || local->tm_year < -1899)
    {
        return STATUS_SYSTEM_TIME_ERROR;
    }
    *key = compose_key((unsigned)(local->tm_year + 1900),
                       (unsigned)(local->tm_mon + 1), (unsigned)local->tm_mday,
                       (unsigned)local->tm_hour, (unsigned)local->tm_min,
                       (unsigned)local->tm_sec);
    return STATUS_OK;
}

void address_destroy(Address *address)
{
    if (address == NULL)
    {
        return;
    }
    string_destroy(&address->city);
    string_destroy(&address->street);
    string_destroy(&address->building);
    string_destroy(&address->index);
    address->house = 0;
    address->apartment = 0;
}

status_t address_create(Address *result, const char *city, const char *street,
                        int house, const char *building, int apartment,
                        const char *index)
{
    if (result == NULL || city == NULL || street == NULL || building == NULL ||
        index == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    memset(result, 0, sizeof(*result));
    if (is_blank(city) || is_blank(street) || house <= 0 || apartment <= 0 ||
        !is_digit_string(index, INDEX_LENGTH))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status_t status = string_create(&result->city, city);
    if (status == STATUS_OK)
    {
        status = string_create(&result->street, street);
    }
    if (status == STATUS_OK)
    {
        status = string_create(&result->building, building);
    }
    if (status == STATUS_OK)
    {
        status = string_create(&result->index, index);
    }
    if (status != STATUS_OK)
    {
        address_destroy(result);
        return status;
    }
    result->house = house;
    result->apartment = apartment;
    return STATUS_OK;
}

status_t address_clone(Address *result, const Address *source)
{
    if (result == NULL || source == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (result == source)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    memset(result, 0, sizeof(*result));

    status_t status = string_copy_into(&result->city, &source->city);
    if (status == STATUS_OK)
    {
        status = string_copy_into(&result->street, &source->street);
    }
    if (status == STATUS_OK)
    {
        status = string_copy_into(&result->building, &source->building);
    }
    if (status == STATUS_OK)
    {
        status = string_copy_into(&result->index, &source->index);
    }
    if (status != STATUS_OK)
    {
        address_destroy(result);
        return status;
    }
    result->house = source->house;
    result->apartment = source->apartment;
    return STATUS_OK;
}

void mail_destroy(Mail *mail)
{
    if (mail == NULL)
    {
        return;
    }
    address_destroy(&mail->recipient);
    string_destroy(&mail->mail_id);
    string_destroy(&mail->created_at);
    string_destroy(&mail->delivery_time);
    mail->weight = 0.0;
    mail->created_key = 0;
    mail->delivery_key = 0;
    mail->delivered = false;
}

status_t mail_create(Mail *result, const Address *recipient, double weight,
                     const char *mail_id, const char *created_at,
                     const char *delivery_time, double epsilon)
{
    if (result == NULL || recipient == NULL || mail_id == NULL ||
        created_at == NULL || delivery_time == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    memset(result, 0, sizeof(*result));

    if (!isfinite(weight) || !isfinite(epsilon) || epsilon < 0.0 ||
        real_less(weight, 0.0, epsilon) || !is_digit_string(mail_id, MAIL_ID_LENGTH))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    uint64_t created_key = 0;
    uint64_t delivery_key = 0;
    status_t status = datetime_parse(created_at, &created_key);
    if (status != STATUS_OK)
    {
        return status;
    }
    status = datetime_parse(delivery_time, &delivery_key);
    if (status != STATUS_OK)
    {
        return status;
    }
    if (delivery_key < created_key)
    {
        return STATUS_INVALID_TIME_ORDER;
    }

    status = address_clone(&result->recipient, recipient);
    if (status == STATUS_OK)
    {
        status = string_create(&result->mail_id, mail_id);
    }
    if (status == STATUS_OK)
    {
        status = string_create(&result->created_at, created_at);
    }
    if (status == STATUS_OK)
    {
        status = string_create(&result->delivery_time, delivery_time);
    }
    if (status != STATUS_OK)
    {
        mail_destroy(result);
        return status;
    }
    result->weight = weight < 0.0 ? 0.0 : weight;
    result->created_key = created_key;
    result->delivery_key = delivery_key;
    result->delivered = false;
    return STATUS_OK;
}

status_t post_init(Post *post, const Address *address)
{
    if (post == NULL || address == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    post->address = NULL;
    post->mails = NULL;
    post->count = 0;
    post->capacity = 0;

    Address *copy = (Address *)malloc(sizeof(Address));
    if (copy == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    status_t status = address_clone(copy, address);
    if (status != STATUS_OK)
    {
        free(copy);
        return status;
    }
    post->address = copy;
    return STATUS_OK;
}

void post_destroy(Post *post)
{
    if (post == NULL)
    {
        return;
    }
    for (size_t i = 0; i < post->count; i++)
    {
        mail_destroy(&post->mails[i]);
    }
    free(post->mails);
    post->mails = NULL;
    post->count = 0;
    post->capacity = 0;
    if (post->address != NULL)
    {
        address_destroy(post->address);
        free(post->address);
        post->address = NULL;
    }
}

static status_t mail_compare(const Mail *left, const Mail *right, int *order)
{
    status_t status =
        string_compare_lex(&left->recipient.index, &right->recipient.index, order);
    if (status != STATUS_OK || *order != 0)
    {
        return status;
    }
    return string_compare_lex(&left->mail_id, &right->mail_id, order);
}

static status_t ensure_capacity(Post *post)
{
    if (post->count < post->capacity)
    {
        return STATUS_OK;
    }
    size_t new_capacity = INITIAL_CAPACITY;
    if (post->capacity != 0)
    {
        if (post->capacity > SIZE_MAX / 2 / sizeof(Mail))
        {
            return STATUS_OVERFLOW;
        }
        new_capacity = post->capacity * 2;
    }
    Mail *grown = (Mail *)realloc(post->mails, new_capacity * sizeof(Mail));
    if (grown == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    post->mails = grown;
    post->capacity = new_capacity;
    return STATUS_OK;
}

status_t post_add_mail(Post *post, Mail *mail)
{
    if (post == NULL || mail == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (mail->mail_id.data == NULL || mail->recipient.index.data == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t position = post->count;
    bool position_found = false;
    for (size_t i = 0; i < post->count; i++)
    {
        bool same = false;
        status_t status =
            string_equal(&post->mails[i].mail_id, &mail->mail_id, &same);
        if (status != STATUS_OK)
        {
            return status;
        }
        if (same)
        {
            return STATUS_DUPLICATE_ID;
        }
        if (!position_found)
        {
            int order = 0;
            status = mail_compare(mail, &post->mails[i], &order);
            if (status != STATUS_OK)
            {
                return status;
            }
            if (order < 0)
            {
                position = i;
                position_found = true;
            }
        }
    }

    status_t status = ensure_capacity(post);
    if (status != STATUS_OK)
    {
        return status;
    }
    memmove(&post->mails[position + 1], &post->mails[position],
            (post->count - position) * sizeof(Mail));
    post->mails[position] = *mail;
    memset(mail, 0, sizeof(*mail));
    post->count++;
    return STATUS_OK;
}

static status_t find_index(const Post *post, const char *mail_id, size_t *index)
{
    if (post == NULL || mail_id == NULL || index == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (!is_digit_string(mail_id, MAIL_ID_LENGTH))
    {
        return STATUS_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < post->count; i++)
    {
        if (strcmp(post->mails[i].mail_id.data, mail_id) == 0)
        {
            *index = i;
            return STATUS_OK;
        }
    }
    return STATUS_NOT_FOUND;
}

status_t post_remove_mail(Post *post, const char *mail_id)
{
    size_t index = 0;
    status_t status = find_index(post, mail_id, &index);
    if (status != STATUS_OK)
    {
        return status;
    }
    mail_destroy(&post->mails[index]);
    memmove(&post->mails[index], &post->mails[index + 1],
            (post->count - index - 1) * sizeof(Mail));
    post->count--;
    memset(&post->mails[post->count], 0, sizeof(Mail));
    return STATUS_OK;
}

status_t post_find_mail(const Post *post, const char *mail_id,
                        const Mail **result)
{
    if (result == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *result = NULL;
    size_t index = 0;
    status_t status = find_index(post, mail_id, &index);
    if (status != STATUS_OK)
    {
        return status;
    }
    *result = &post->mails[index];
    return STATUS_OK;
}

status_t post_mark_delivered(Post *post, const char *mail_id)
{
    size_t index = 0;
    status_t status = find_index(post, mail_id, &index);
    if (status != STATUS_OK)
    {
        return status;
    }
    if (post->mails[index].delivered)
    {
        return STATUS_ALREADY_DELIVERED;
    }
    post->mails[index].delivered = true;
    return STATUS_OK;
}

static bool mail_matches(const Mail *mail, mail_filter_t filter,
                         uint64_t now_key)
{
    if (filter == MAIL_FILTER_DELIVERED)
    {
        return mail->delivered;
    }
    return !mail->delivered && mail->delivery_key < now_key;
}

static int compare_by_creation(const void *left, const void *right)
{
    const Mail *first = *(const Mail *const *)left;
    const Mail *second = *(const Mail *const *)right;
    if (first->created_key != second->created_key)
    {
        return first->created_key < second->created_key ? -1 : 1;
    }
    int order = 0;
    if (string_compare_lex(&first->mail_id, &second->mail_id, &order) !=
        STATUS_OK)
    {
        return 0;
    }
    return order;
}

status_t post_select_mails(const Post *post, mail_filter_t filter,
                           uint64_t now_key, const Mail ***result,
                           size_t *count)
{
    if (post == NULL || result == NULL || count == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *result = NULL;
    *count = 0;
    if (filter != MAIL_FILTER_DELIVERED && filter != MAIL_FILTER_EXPIRED)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t matches = 0;
    for (size_t i = 0; i < post->count; i++)
    {
        if (mail_matches(&post->mails[i], filter, now_key))
        {
            matches++;
        }
    }
    if (matches == 0)
    {
        return STATUS_OK;
    }
    if (matches > SIZE_MAX / sizeof(const Mail *))
    {
        return STATUS_OVERFLOW;
    }

    const Mail **items = (const Mail **)malloc(matches * sizeof(const Mail *));
    if (items == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    size_t filled = 0;
    for (size_t i = 0; i < post->count; i++)
    {
        if (mail_matches(&post->mails[i], filter, now_key))
        {
            items[filled] = &post->mails[i];
            filled++;
        }
    }
    qsort(items, matches, sizeof(const Mail *), compare_by_creation);
    *result = items;
    *count = matches;
    return STATUS_OK;
}