#ifndef POST_H
#define POST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mystring.h"
#include "status.h"

#define INDEX_LENGTH 6
#define MAIL_ID_LENGTH 14
#define DATETIME_LENGTH 19

typedef struct
{
    String city;
    String street;
    int house;
    String building;
    int apartment;
    String index;
} Address;

typedef struct
{
    Address recipient;
    double weight;
    String mail_id;
    String created_at;
    String delivery_time;
    uint64_t created_key;
    uint64_t delivery_key;
    bool delivered;
} Mail;

typedef struct
{
    Address *address;
    Mail *mails;
    size_t count;
    size_t capacity;
} Post;

typedef enum
{
    MAIL_FILTER_DELIVERED,
    MAIL_FILTER_EXPIRED
} mail_filter_t;

status_t datetime_parse(const char *text, uint64_t *key);
status_t datetime_now_key(uint64_t *key);

status_t address_create(Address *result, const char *city, const char *street,
                        int house, const char *building, int apartment,
                        const char *index);
status_t address_clone(Address *result, const Address *source);
void address_destroy(Address *address);

status_t mail_create(Mail *result, const Address *recipient, double weight,
                     const char *mail_id, const char *created_at,
                     const char *delivery_time, double epsilon);
void mail_destroy(Mail *mail);

status_t post_init(Post *post, const Address *address);
void post_destroy(Post *post);
status_t post_add_mail(Post *post, Mail *mail);
status_t post_remove_mail(Post *post, const char *mail_id);
status_t post_find_mail(const Post *post, const char *mail_id,
                        const Mail **result);
status_t post_mark_delivered(Post *post, const char *mail_id);
status_t post_select_mails(const Post *post, mail_filter_t filter,
                           uint64_t now_key, const Mail ***result,
                           size_t *count);

#endif