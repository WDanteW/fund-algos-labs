#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_R 5u
#define BUF_SIZE 34u
#define SAMPLES_COUNT 10u
#define BUF_END 33u

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_RADIX_POWER,
    STATUS_INVALID_INPUT,
    STATUS_OVERFLOW,
    STATUS_NO_MEMORY
} status_t;

static const char DIGITS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

static uint32_t bit_add(uint32_t a, uint32_t b)
{
    while (b != 0u)
    {
        uint32_t carry = (a & b) << 1;
        a ^= b;
        b = carry;
    }
    return a;
}

static uint32_t bit_negate(uint32_t x)
{
    return bit_add(~x, 1u);
}

static uint32_t bit_dec(uint32_t x)
{
    return bit_add(x, ~0u);
}

static uint32_t bit_inc(uint32_t x)
{
    return bit_add(x, 1u);
}

static status_t int_to_base_2r(int number, unsigned int r, char **result)
{
    if (result == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *result = NULL;
    if (r < 1u || r > MAX_R)
    {
        return STATUS_INVALID_RADIX_POWER;
    }

    char *buf = (char *)malloc(BUF_SIZE);
    if (buf == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    uint32_t mask = ~(~0u << r);
    uint32_t negative = ((uint32_t)number) >> 31;
    uint32_t value = negative ? bit_negate((uint32_t)number) : (uint32_t)number;

    uint32_t pos = BUF_END;
    buf[pos] = '\0';

    if (value == 0u)
    {
        pos = bit_dec(pos);
        buf[pos] = '0';
    }
    while (value != 0u)
    {
        pos = bit_dec(pos);
        buf[pos] = DIGITS[value & mask];
        value >>= r;
    }
    if (negative)
    {
        pos = bit_dec(pos);
        buf[pos] = '-';
    }

    uint32_t len = bit_add(BUF_END, bit_negate(pos));
    char *out = (char *)malloc(bit_inc(len));
    if (out == NULL)
    {
        free(buf);
        return STATUS_NO_MEMORY;
    }

    uint32_t i = 0u;
    while ((out[i] = buf[pos]) != '\0')
    {
        i = bit_inc(i);
        pos = bit_inc(pos);
    }

    free(buf);
    *result = out;
    return STATUS_OK;
}

static status_t parse_decimal_int(const char *str, int *out)
{
    if (str == NULL || out == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    uint32_t i = 0u;
    uint32_t negative = 0u;
    if (str[i] == '-')
    {
        negative = 1u;
        i = bit_inc(i);
    }
    if (str[i] == '\0')
    {
        return STATUS_INVALID_INPUT;
    }

    uint32_t acc = 0u;
    while (str[i] != '\0')
    {
        char c = str[i];
        if (c < '0' || c > '9')
        {
            return STATUS_INVALID_INPUT;
        }
        if (acc > 214748364u)
        {
            return STATUS_OVERFLOW;
        }
        uint32_t times10 = bit_add(acc << 3, acc << 1);
        acc = bit_add(times10, (uint32_t)(c & 0x0F));
        if (acc > 2147483648u)
        {
            return STATUS_OVERFLOW;
        }
        i = bit_inc(i);
    }

    if (negative)
    {
        *out = (int)bit_negate(acc);
    }
    else
    {
        if (acc > 2147483647u)
        {
            return STATUS_OVERFLOW;
        }
        *out = (int)acc;
    }
    return STATUS_OK;
}

static status_t parse_radix_power(const char *str, unsigned int *r)
{
    int value = 0;
    status_t st = parse_decimal_int(str, &value);
    if (st != STATUS_OK)
    {
        return st;
    }
    if (value < 1 || value > (int)MAX_R)
    {
        return STATUS_INVALID_RADIX_POWER;
    }
    *r = (unsigned int)value;
    return STATUS_OK;
}

static const char *status_message(status_t st)
{
    switch (st)
    {
    case STATUS_OK:                  return "успех";
    case STATUS_NULL_POINTER:        return "нулевой указатель";
    case STATUS_INVALID_RADIX_POWER: return "r должно быть целым от 1 до 5";
    case STATUS_INVALID_INPUT:       return "некорректное десятичное число";
    case STATUS_OVERFLOW:            return "число не помещается в int";
    case STATUS_NO_MEMORY:           return "не удалось выделить память";
    default:                         return "неизвестная ошибка";
    }
}

static status_t print_conversions(int number)
{
    printf("Число %d:\n", number);
    for (unsigned int r = 1u; r <= MAX_R; r = bit_inc(r))
    {
        char *text = NULL;
        status_t st = int_to_base_2r(number, r, &text);
        if (st != STATUS_OK)
        {
            return st;
        }

        long back = strtol(text, NULL, (int)(1u << r));
        printf("  r=%u (основание %2u): %-34s %s\n", r, 1u << r, text,
               back == (long)number ? "[проверка OK]" : "[ОШИБКА]");
        free(text);
    }
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        static const int samples[] = {0,    1,      5,          10,
                                      255,  -255,   1024,       12345,
                                      2147483647, INT32_MIN};
        for (size_t i = 0; i < SAMPLES_COUNT; i = bit_inc((uint32_t)i))
        {
            status_t st = print_conversions(samples[i]);
            if (st != STATUS_OK)
            {
                fprintf(stderr, "Ошибка: %s\n", status_message(st));
                return 1;
            }
        }
        return 0;
    }

    if (argc != 3)
    {
        fprintf(stderr, "Использование: %s [<число> <r>]\n", argv[0]);
        return 1;
    }

    int number = 0;
    unsigned int r = 0u;
    status_t st = parse_decimal_int(argv[1], &number);
    if (st == STATUS_OK)
    {
        st = parse_radix_power(argv[2], &r);
    }
    if (st != STATUS_OK)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(st));
        return 1;
    }

    char *text = NULL;
    st = int_to_base_2r(number, r, &text);
    if (st != STATUS_OK)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(st));
        return 1;
    }
    printf("%d в системе с основанием %u: %s\n", number, 1u << r, text);
    free(text);
    return 0;
}