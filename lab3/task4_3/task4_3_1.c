#include <stdio.h>

#include "mystring.h"
#include "status.h"

static const char *order_text(int order)
{
    if (order < 0)
    {
        return "меньше";
    }
    if (order > 0)
    {
        return "больше";
    }
    return "равно";
}

static status_t demo_create_and_destroy(void)
{
    printf("--- Создание и удаление ---\n");
    String text = {NULL, 0};
    status_t status = string_create(&text, "Hello, world");
    if (status == STATUS_OK)
    {
        printf("Создана строка \"%s\", длина %d\n", text.data, text.length);
    }
    string_destroy(&text);
    if (status == STATUS_OK)
    {
        printf("После удаления: data = %s, length = %d\n",
               text.data == NULL ? "NULL" : "не NULL", text.length);
    }
    return status;
}

static status_t demo_compare(void)
{
    static const char *const pairs[][2] = {
        {"zz", "aaa"}, {"abc", "abd"}, {"abc", "abc"}, {"", "a"},
        {"apple", "Apple"}};
    printf("\n--- Сравнение ---\n");

    status_t status = STATUS_OK;
    for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]) && status == STATUS_OK;
         i++)
    {
        String left = {NULL, 0};
        String right = {NULL, 0};
        int by_order = 0;
        int by_lex = 0;
        bool equal = false;

        status = string_create(&left, pairs[i][0]);
        if (status == STATUS_OK)
        {
            status = string_create(&right, pairs[i][1]);
        }
        if (status == STATUS_OK)
        {
            status = string_compare_order(&left, &right, &by_order);
        }
        if (status == STATUS_OK)
        {
            status = string_compare_lex(&left, &right, &by_lex);
        }
        if (status == STATUS_OK)
        {
            status = string_equal(&left, &right, &equal);
        }
        if (status == STATUS_OK)
        {
            printf("\"%s\" и \"%s\": по длине и алфавиту - %s, "
                   "лексикографически - %s, эквивалентны - %s\n",
                   left.data, right.data, order_text(by_order),
                   order_text(by_lex), equal ? "да" : "нет");
        }
        string_destroy(&left);
        string_destroy(&right);
    }
    return status;
}

static status_t demo_copy(void)
{
    printf("\n--- Копирование ---\n");
    String source = {NULL, 0};
    String target = {NULL, 0};
    String *clone = NULL;
    String suffix = {NULL, 0};

    status_t status = string_create(&source, "source text");
    if (status == STATUS_OK)
    {
        status = string_create(&target, "old value");
    }
    if (status == STATUS_OK)
    {
        printf("До копирования: target = \"%s\"\n", target.data);
        status = string_copy_into(&target, &source);
    }
    if (status == STATUS_OK)
    {
        printf("После копирования в существующую: target = \"%s\" (длина %d)\n",
               target.data, target.length);
        status = string_clone(&clone, &source);
    }
    if (status == STATUS_OK)
    {
        printf("Клон в динамической памяти: \"%s\" (длина %d)\n", clone->data,
               clone->length);
        status = string_create(&suffix, " + changed");
    }
    if (status == STATUS_OK)
    {
        status = string_concat(&source, &suffix);
    }
    if (status == STATUS_OK)
    {
        printf("Исходная после изменения: \"%s\"\n", source.data);
        printf("Копия осталась прежней: \"%s\"\n", clone->data);
    }

    string_destroy(&source);
    string_destroy(&target);
    string_destroy(&suffix);
    string_free(&clone);
    return status;
}

static status_t demo_concat(void)
{
    printf("\n--- Конкатенация ---\n");
    String first = {NULL, 0};
    String second = {NULL, 0};
    String twin = {NULL, 0};

    status_t status = string_create(&first, "Hello");
    if (status == STATUS_OK)
    {
        status = string_create(&second, ", world");
    }
    if (status == STATUS_OK)
    {
        status = string_concat(&first, &second);
    }
    if (status == STATUS_OK)
    {
        printf("\"Hello\" + \", world\" = \"%s\" (длина %d)\n", first.data,
               first.length);
        status = string_create(&twin, "ab");
    }
    if (status == STATUS_OK)
    {
        status = string_concat(&twin, &twin);
    }
    if (status == STATUS_OK)
    {
        printf("Конкатенация строки с самой собой: \"%s\" (длина %d)\n",
               twin.data, twin.length);
    }

    string_destroy(&first);
    string_destroy(&second);
    string_destroy(&twin);
    return status;
}

static status_t demo_errors(void)
{
    printf("\n--- Обработка ошибок ---\n");
    String empty = {NULL, 0};
    String valid = {NULL, 0};
    int order = 0;

    status_t status = string_create(&valid, "text");
    if (status != STATUS_OK)
    {
        return status;
    }
    printf("Создание из NULL: %s\n",
           status_message(string_create(&empty, NULL)));
    printf("Сравнение с неинициализированной строкой: %s\n",
           status_message(string_compare_order(&valid, &empty, &order)));
    printf("Конкатенация с неинициализированной строкой: %s\n",
           status_message(string_concat(&valid, &empty)));
    string_destroy(&valid);
    return STATUS_OK;
}

int main(void)
{
    status_t status = demo_create_and_destroy();
    if (status == STATUS_OK)
    {
        status = demo_compare();
    }
    if (status == STATUS_OK)
    {
        status = demo_copy();
    }
    if (status == STATUS_OK)
    {
        status = demo_concat();
    }
    if (status == STATUS_OK)
    {
        status = demo_errors();
    }
    if (status != STATUS_OK)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(status));
        return 1;
    }
    return 0;
}