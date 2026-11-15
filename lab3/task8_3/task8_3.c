#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_NO_MEMORY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_OVERFLOW,
    STATUS_DIVISION_BY_ZERO
} status_t;

typedef struct term {
    long long coef;
    int degree;
    struct term *next;
} term_t;

typedef struct {
    term_t *head;
} polynomial_t;

static polynomial_t *poly_create(void) {
    polynomial_t *p = (polynomial_t *)malloc(sizeof(polynomial_t));
    if (p) p->head = NULL;
    return p;
}

static void poly_free(polynomial_t *p) {
    if (!p) return;
    term_t *curr = p->head;
    while (curr) {
        term_t *next = curr->next;
        free(curr);
        curr = next;
    }
    free(p);
}

static status_t poly_add_term(polynomial_t *p, long long coef, int degree) {
    if (coef == 0) return STATUS_OK;
    if (!p) return STATUS_NULL_POINTER;
    if (degree < 0 || degree > 1000) return STATUS_INVALID_INPUT;

    term_t *prev = NULL;
    term_t *curr = p->head;
    while (curr && curr->degree > degree) {
        prev = curr;
        curr = curr->next;
    }
    if (curr && curr->degree == degree) {
        if ((coef > 0 && curr->coef > LLONG_MAX - coef) ||
            (coef < 0 && curr->coef < LLONG_MIN - coef)) {
            return STATUS_OVERFLOW;
        }
        curr->coef += coef;
        if (curr->coef == 0) {
            if (prev) prev->next = curr->next;
            else p->head = curr->next;
            free(curr);
        }
        return STATUS_OK;
    }
    term_t *new_term = (term_t *)malloc(sizeof(term_t));
    if (!new_term) return STATUS_NO_MEMORY;
    new_term->coef = coef;
    new_term->degree = degree;
    new_term->next = curr;
    if (prev) prev->next = new_term;
    else p->head = new_term;
    return STATUS_OK;
}

static status_t poly_copy(const polynomial_t *src, polynomial_t **dest_out) {
    if (!src || !dest_out) return STATUS_NULL_POINTER;
    polynomial_t *dest = poly_create();
    if (!dest) return STATUS_NO_MEMORY;

    term_t *curr = src->head;
    while (curr) {
        status_t st = poly_add_term(dest, curr->coef, curr->degree);
        if (st != STATUS_OK) {
            poly_free(dest);
            return st;
        }
        curr = curr->next;
    }
    *dest_out = dest;
    return STATUS_OK;
}

static int poly_degree(const polynomial_t *p) {
    if (!p || !p->head) return -1;
    return p->head->degree;
}

static long long poly_leading_coef(const polynomial_t *p) {
    if (!p || !p->head) return 0;
    return p->head->coef;
}

static status_t poly_parse(const char **str_ptr, polynomial_t **out_poly) {
    if (!str_ptr || !out_poly) return STATUS_NULL_POINTER;
    polynomial_t *p = poly_create();
    if (!p) return STATUS_NO_MEMORY;

    const char *s = *str_ptr;
    bool expect_term = true;

    while (*s && *s != ')' && *s != ',' && *s != ';') {
        while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
        if (!*s || *s == ')' || *s == ',' || *s == ';') break;

        if (!expect_term) {
            poly_free(p);
            return STATUS_INVALID_INPUT;
        }

        long long coef = 1;
        int degree = 0;
        int sign = 1;

        if (*s == '+') {
            sign = 1;
            s++;
        } else if (*s == '-') {
            sign = -1;
            s++;
        }

        bool has_digits = false;
        long long c_val = 0;
        while (*s >= '0' && *s <= '9') {
            long long digit = *s - '0';
            if (c_val > (LLONG_MAX - digit) / 10) {
                poly_free(p);
                return STATUS_OVERFLOW;
            }
            c_val = c_val * 10 + digit;
            has_digits = true;
            s++;
        }

        if (has_digits) coef = c_val;
        coef *= sign;

        if (*s == 'x' || *s == 'X') {
            s++;
            if (!has_digits) {
                coef = sign;
            }
            degree = 1;
            if (*s == '^') {
                s++;
                int d_sign = 1;
                if (*s == '+') s++;
                else if (*s == '-') {
                    d_sign = -1;
                    s++;
                }
                int d_val = 0;
                bool has_deg_digits = false;
                while (*s >= '0' && *s <= '9') {
                    int digit = *s - '0';
                    if (d_val > (INT_MAX - digit) / 10) {
                        poly_free(p);
                        return STATUS_OVERFLOW;
                    }
                    d_val = d_val * 10 + digit;
                    has_deg_digits = true;
                    s++;
                }
                if (!has_deg_digits) {
                    poly_free(p);
                    return STATUS_INVALID_INPUT;
                }
                degree = d_val * d_sign;
            }
        } else {
            if (!has_digits) {
                poly_free(p);
                return STATUS_INVALID_INPUT;
            }
            degree = 0;
        }

        status_t st = poly_add_term(p, coef, degree);
        if (st != STATUS_OK) {
            poly_free(p);
            return st;
        }

        while (*s && (*s == ' ' || *s == '\t')) s++;
        if (*s && *s != ')' && *s != ',' && *s != ';' && *s != '+' && *s != '-') {
            poly_free(p);
            return STATUS_INVALID_INPUT;
        }
        expect_term = false;
        if (*s == '+' || *s == '-') {
            expect_term = true;
        }
    }

    *str_ptr = s;
    *out_poly = p;
    return STATUS_OK;
}

static void poly_print(FILE *stream, const polynomial_t *p) {
    if (!stream) return;
    if (!p || !p->head) {
        fprintf(stream, "0\n");
        return;
    }
    bool first = true;
    term_t *curr = p->head;
    while (curr) {
        long long c = curr->coef;
        int d = curr->degree;
        if (c > 0) {
            if (!first) fprintf(stream, "+");
        } else if (c < 0) {
            fprintf(stream, "-");
            c = -c;
        }
        if (d == 0) {
            fprintf(stream, "%lld", c);
        } else {
            if (c != 1) fprintf(stream, "%lld", c);
            fprintf(stream, "x");
            if (d != 1) fprintf(stream, "^%d", d);
        }
        first = false;
        curr = curr->next;
    }
    fprintf(stream, "\n");
}

static status_t poly_add(const polynomial_t *a, const polynomial_t *b, polynomial_t **out) {
    if (!a || !b || !out) return STATUS_NULL_POINTER;
    polynomial_t *res = NULL;
    status_t st = poly_copy(a, &res);
    if (st != STATUS_OK) return st;

    term_t *curr = b->head;
    while (curr) {
        st = poly_add_term(res, curr->coef, curr->degree);
        if (st != STATUS_OK) {
            poly_free(res);
            return st;
        }
        curr = curr->next;
    }
    *out = res;
    return STATUS_OK;
}

static status_t poly_sub(const polynomial_t *a, const polynomial_t *b, polynomial_t **out) {
    if (!a || !b || !out) return STATUS_NULL_POINTER;
    polynomial_t *res = NULL;
    status_t st = poly_copy(a, &res);
    if (st != STATUS_OK) return st;

    term_t *curr = b->head;
    while (curr) {
        if (curr->coef == LLONG_MIN) {
            poly_free(res);
            return STATUS_OVERFLOW;
        }
        st = poly_add_term(res, -curr->coef, curr->degree);
        if (st != STATUS_OK) {
            poly_free(res);
            return st;
        }
        curr = curr->next;
    }
    *out = res;
    return STATUS_OK;
}

static status_t poly_mul(const polynomial_t *a, const polynomial_t *b, polynomial_t **out) {
    if (!a || !b || !out) return STATUS_NULL_POINTER;
    polynomial_t *res = poly_create();
    if (!res) return STATUS_NO_MEMORY;

    term_t *ta = a->head;
    while (ta) {
        term_t *tb = b->head;
        while (tb) {
            if (ta->coef != 0 && tb->coef != 0) {
                if ((ta->coef > 0 && tb->coef > 0 && ta->coef > LLONG_MAX / tb->coef) ||
                    (ta->coef > 0 && tb->coef < 0 && tb->coef < LLONG_MIN / ta->coef) ||
                    (ta->coef < 0 && tb->coef > 0 && ta->coef < LLONG_MIN / tb->coef) ||
                    (ta->coef < 0 && tb->coef < 0 && ta->coef < LLONG_MAX / tb->coef)) {
                    poly_free(res);
                    return STATUS_OVERFLOW;
                }
            }
            status_t st = poly_add_term(res, ta->coef * tb->coef, ta->degree + tb->degree);
            if (st != STATUS_OK) {
                poly_free(res);
                return st;
            }
            tb = tb->next;
        }
        ta = ta->next;
    }
    *out = res;
    return STATUS_OK;
}

static status_t poly_div_mod(const polynomial_t *dividend, const polynomial_t *divisor, polynomial_t **quotient_out, polynomial_t **remainder_out) {
    if (!dividend || !divisor) return STATUS_NULL_POINTER;
    if (!divisor->head) return STATUS_DIVISION_BY_ZERO;

    polynomial_t *q = poly_create();
    polynomial_t *r = NULL;
    status_t st = poly_copy(dividend, &r);
    if (!q || st != STATUS_OK) {
        poly_free(q);
        poly_free(r);
        return STATUS_NO_MEMORY;
    }

    int deg_b = poly_degree(divisor);
    long long lc_b = poly_leading_coef(divisor);

    while (r->head && poly_degree(r) >= deg_b) {
        int deg_r = poly_degree(r);
        long long lc_r = poly_leading_coef(r);
        if (lc_r == LLONG_MIN && lc_b == -1) {
            poly_free(q);
            poly_free(r);
            return STATUS_OVERFLOW;
        }
        if (lc_r % lc_b != 0) break;

        long long c_coef = lc_r / lc_b;
        int c_deg = deg_r - deg_b;

        st = poly_add_term(q, c_coef, c_deg);
        if (st != STATUS_OK) {
            poly_free(q);
            poly_free(r);
            return st;
        }

        polynomial_t *sub_term = poly_create();
        poly_add_term(sub_term, c_coef, c_deg);

        polynomial_t *product = NULL;
        st = poly_mul(sub_term, divisor, &product);
        poly_free(sub_term);
        if (st != STATUS_OK) {
            poly_free(q);
            poly_free(r);
            return st;
        }

        polynomial_t *new_r = NULL;
        st = poly_sub(r, product, &new_r);
        poly_free(product);
        poly_free(r);
        if (st != STATUS_OK) {
            poly_free(q);
            return st;
        }
        r = new_r;
    }

    if (quotient_out) *quotient_out = q;
    else poly_free(q);
    if (remainder_out) *remainder_out = r;
    else poly_free(r);
    return STATUS_OK;
}

static status_t poly_eval(const polynomial_t *p, long long x, long long *out_val) {
    if (!p || !out_val) return STATUS_NULL_POINTER;
    if (!p->head) {
        *out_val = 0;
        return STATUS_OK;
    }

    long long res = 0;
    int current_deg = p->head->degree;
    term_t *curr = p->head;

    unsigned long long abs_x = (x == LLONG_MIN) ? (unsigned long long)LLONG_MAX + 1 : (unsigned long long)llabs(x);

    while (curr) {
        while (current_deg > curr->degree) {
            if (abs_x != 0 && (res > LLONG_MAX / (long long)abs_x || res < LLONG_MIN / (long long)abs_x)) {
                return STATUS_OVERFLOW;
            }
            res *= x;
            current_deg--;
        }
        if ((curr->coef > 0 && res > LLONG_MAX - curr->coef) ||
            (curr->coef < 0 && res < LLONG_MIN - curr->coef)) {
            return STATUS_OVERFLOW;
        }
        res += curr->coef;
        curr = curr->next;
    }
    while (current_deg > 0) {
        if (abs_x != 0 && (res > LLONG_MAX / (long long)abs_x || res < LLONG_MIN / (long long)abs_x)) {
            return STATUS_OVERFLOW;
        }
        res *= x;
        current_deg--;
    }

    *out_val = res;
    return STATUS_OK;
}

static status_t poly_diff(const polynomial_t *p, polynomial_t **out) {
    if (!p || !out) return STATUS_NULL_POINTER;
    polynomial_t *res = poly_create();
    if (!res) return STATUS_NO_MEMORY;

    term_t *curr = p->head;
    while (curr) {
        if (curr->degree > 0) {
            if (curr->coef != 0) {
                if ((curr->coef > 0 && (curr->degree > LLONG_MAX / curr->coef)) ||
                    (curr->coef < 0 && (curr->coef < LLONG_MIN / curr->degree))) {
                    poly_free(res);
                    return STATUS_OVERFLOW;
                }
            }
            status_t st = poly_add_term(res, curr->coef * curr->degree, curr->degree - 1);
            if (st != STATUS_OK) {
                poly_free(res);
                return st;
            }
        }
        curr = curr->next;
    }
    *out = res;
    return STATUS_OK;
}

static status_t poly_compose(const polynomial_t *a, const polynomial_t *b, polynomial_t **out) {
    if (!a || !b || !out) return STATUS_NULL_POINTER;
    polynomial_t *res = poly_create();
    if (!res) return STATUS_NO_MEMORY;

    term_t *curr = a->head;
    while (curr) {
        polynomial_t *term_poly = poly_create();
        status_t st = poly_add_term(term_poly, curr->coef, 0);
        if (st != STATUS_OK) {
            poly_free(term_poly);
            poly_free(res);
            return st;
        }

        int d = curr->degree;
        polynomial_t *b_pow = NULL;
        const char *one_str = "1";
        const char *p_one = one_str;
        st = poly_parse(&p_one, &b_pow);
        if (st != STATUS_OK) {
            poly_free(term_poly);
            poly_free(res);
            return st;
        }

        for (int i = 0; i < d; i++) {
            polynomial_t *next_pow = NULL;
            st = poly_mul(b_pow, b, &next_pow);
            poly_free(b_pow);
            b_pow = next_pow;
            if (st != STATUS_OK) {
                poly_free(term_poly);
                poly_free(res);
                return st;
            }
        }

        polynomial_t *product = NULL;
        st = poly_mul(term_poly, b_pow, &product);
        poly_free(term_poly);
        poly_free(b_pow);
        if (st != STATUS_OK) {
            poly_free(res);
            return st;
        }

        polynomial_t *new_res = NULL;
        st = poly_add(res, product, &new_res);
        poly_free(product);
        poly_free(res);
        if (st != STATUS_OK) return st;
        res = new_res;

        curr = curr->next;
    }

    *out = res;
    return STATUS_OK;
}

static status_t run_interpreter(const char *filename) {
    if (!filename) return STATUS_NULL_POINTER;
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Ошибка: не удалось открыть файл '%s'\n", filename);
        return STATUS_FILE_OPEN_ERROR;
    }

    polynomial_t *accumulator = NULL;
    const char *zero_str = "0";
    const char *p_zero = zero_str;
    status_t st = poly_parse(&p_zero, &accumulator);
    if (st != STATUS_OK) {
        fclose(f);
        return st;
    }

    char line[1024];
    bool in_multiline_comment = false;

    while (fgets(line, sizeof(line), f)) {
        const char *p = line;
        while (*p) {
            if (in_multiline_comment) {
                if (*p == ']') {
                    in_multiline_comment = false;
                }
                p++;
                continue;
            }
            if (*p == '[') {
                in_multiline_comment = true;
                p++;
                continue;
            }
            if (*p == '%') {
                break;
            }
            if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
                p++;
                continue;
            }

            char cmd[32];
            int i = 0;
            while (*p && isalpha((unsigned char)*p) && i < 31) {
                cmd[i++] = *p++;
            }
            cmd[i] = '\0';
            if (i == 0) {
                p++;
                continue;
            }

            while (*p && *p != '(') p++;
            if (*p == '(') p++;

            polynomial_t *arg1 = NULL;
            polynomial_t *arg2 = NULL;
            long long eval_arg = 0;
            bool is_eval = (strcmp(cmd, "Eval") == 0);

            if (is_eval) {
                while (*p && (*p == ' ' || *p == '\t')) p++;
                long long val = 0;
                int sign = 1;
                if (*p == '-') {
                    sign = -1;
                    p++;
                } else if (*p == '+') {
                    p++;
                }
                bool has_eval_digits = false;
                while (*p >= '0' && *p <= '9') {
                    long long digit = *p - '0';
                    if (val > (LLONG_MAX - digit) / 10) {
                        poly_free(accumulator);
                        fclose(f);
                        return STATUS_OVERFLOW;
                    }
                    val = val * 10 + digit;
                    has_eval_digits = true;
                    p++;
                }
                while (*p && (*p == ' ' || *p == '\t')) p++;
                if (!has_eval_digits || (*p != ')' && *p != ';')) {
                    poly_free(accumulator);
                    fclose(f);
                    return STATUS_INVALID_INPUT;
                }
                eval_arg = val * sign;
            } else {
                const char *saved_p = p;
                st = poly_parse(&p, &arg1);
                if (st != STATUS_OK) {
                    poly_free(accumulator);
                    fclose(f);
                    return st;
                }
                while (*p && (*p == ' ' || *p == '\t')) p++;
                if (*p == ',') {
                    p++;
                    while (*p && (*p == ' ' || *p == '\t')) p++;
                    if (*p == ')') {
                        poly_free(arg1);
                        poly_free(accumulator);
                        fclose(f);
                        return STATUS_INVALID_INPUT;
                    }
                    st = poly_parse(&p, &arg2);
                    if (st != STATUS_OK) {
                        poly_free(arg1);
                        poly_free(accumulator);
                        fclose(f);
                        return st;
                    }
                } else {
                    poly_free(arg1);
                    p = saved_p;
                    st = poly_copy(accumulator, &arg1);
                    if (st != STATUS_OK) {
                        poly_free(accumulator);
                        fclose(f);
                        return st;
                    }
                    st = poly_parse(&p, &arg2);
                    if (st != STATUS_OK) {
                        poly_free(arg1);
                        poly_free(accumulator);
                        fclose(f);
                        return st;
                    }
                }
            }

            while (*p && *p != ')') p++;
            if (*p == ')') p++;
            while (*p && *p != ';') p++;
            if (*p == ';') p++;

            polynomial_t *res = NULL;
            if (strcmp(cmd, "Add") == 0) {
                st = poly_add(arg1, arg2, &res);
            } else if (strcmp(cmd, "Sub") == 0) {
                st = poly_sub(arg1, arg2, &res);
            } else if (strcmp(cmd, "Mult") == 0) {
                st = poly_mul(arg1, arg2, &res);
            } else if (strcmp(cmd, "Div") == 0) {
                st = poly_div_mod(arg1, arg2, &res, NULL);
            } else if (strcmp(cmd, "Mod") == 0) {
                st = poly_div_mod(arg1, arg2, NULL, &res);
            } else if (strcmp(cmd, "Eval") == 0) {
                long long ev = 0;
                st = poly_eval(accumulator, eval_arg, &ev);
                if (st == STATUS_OK) {
                    fprintf(stdout, "%lld\n", ev);
                }
            } else if (strcmp(cmd, "Diff") == 0) {
                st = poly_diff(arg1, &res);
            } else if (strcmp(cmd, "Cmps") == 0) {
                st = poly_compose(arg1, arg2, &res);
            } else {
                fprintf(stderr, "Предупреждение: неизвестная команда '%s'\n", cmd);
                st = STATUS_INVALID_INPUT;
            }

            if (st != STATUS_OK) {
                poly_free(arg1);
                poly_free(arg2);
                poly_free(accumulator);
                fclose(f);
                return st;
            }

            if (res) {
                poly_free(accumulator);
                accumulator = res;
                poly_print(stdout, accumulator);
            }

            if (arg1) poly_free(arg1);
            if (arg2) poly_free(arg2);
        }
    }

    if (ferror(f)) {
        poly_free(accumulator);
        if (fclose(f) != 0) return STATUS_FILE_READ_ERROR;
        return STATUS_FILE_READ_ERROR;
    }

    poly_free(accumulator);
    if (fclose(f) != 0) {
        return STATUS_FILE_READ_ERROR;
    }
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <файл_скрипта>\n", argv[0]);
        return 1;
    }
    status_t st = run_interpreter(argv[1]);
    if (st != STATUS_OK) {
        fprintf(stderr, "Программа завершилась с ошибкой (код: %d)\n", st);
        return 1;
    }
    return 0;
}