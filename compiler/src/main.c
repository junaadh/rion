#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

void *xrealloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("realloc failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xmalloc(size_t num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

#define GROWTH_FACTOR 2
typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) \
    (buf__fits(b, n) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_push(b, ...) \
    (buf__fit(b, 1), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    size_t new_cap = MAX(1 + GROWTH_FACTOR * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
    BufHdr *new_hdr;
    if (buf) {
        new_hdr = xrealloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

void buf_test(void) {
    size_t num = 20;
    size_t *vec = NULL;

    for (size_t i = 0; i < num; i++) {
        buf_push(vec, i);
    }

    assert(num == buf_len(vec));

    // printf("len: %zu, cap: %zu\n", buf_len(vec), buf_cap(vec));
    for (size_t i = 0; i < num; i++) {
        // printf("i = %zu => %zu\n", i, vec[i]);
        assert(i == vec[i]);
    }

    buf_free(vec);
}

typedef struct Intern {
    size_t len;
    const char *str;
} Intern;

static Intern *interns;

const char *str_intern_range(const char *start, const char *end) {
    size_t len = end - start;
    for (Intern *it = interns; it != buf_end(interns); it++) {
        if (it->len == len && strncmp(it->str, start, len) == 0) {
            return it->str;
        }
    }
    char *str = xmalloc(len + 1);
    memcpy(str, start, len);
    str[len] = 0;
    buf_push(interns, (Intern){len, str});
    return str;
}

const char *str_intern(const char *str) {
    return str_intern_range(str, str + strlen(str));
}

void str_intern_test(void) {
    char x[] = "hello";
    char y[] = "hello";
    assert(x != y);
    const char *px = str_intern(x);
    const char *py = str_intern(y);
    assert(px == py);
    char z[] = "hello!";
    const char *pz = str_intern(z);
    assert(px != pz);
}

typedef enum TokenKind {
    TOKEN_EOF = 0,
    TOKEN_INT = 128,
    TOKEN_IDENT,
    // ...
} TokenKind;

size_t copy_token_kind_name(char *dest, size_t dest_size, TokenKind kind) {
    size_t n = 0;
    switch (kind) {
    case TOKEN_EOF:
        n = snprintf(dest, dest_size, "end of file");
        break;
    case TOKEN_INT:
        n = snprintf(dest, dest_size, "integer");
        break;
    case TOKEN_IDENT:
        n = snprintf(dest, dest_size, "identifier");
        break;
    default:
        if (kind < 128 && isprint(kind))
            n = snprintf(dest, dest_size, "%c", kind);
        else
            n = snprintf(dest, dest_size, "<ASCII %d>", kind);
    }
    return n;
}

const char *token_kind_name(TokenKind kind) {
    static char buf[32];
    size_t n = copy_token_kind_name(buf, sizeof(buf), kind);
    assert(n + 1 < sizeof(buf));
    return buf;
}

typedef struct Token {
    TokenKind kind;
    const char *lo;
    const char *hi;
    union {
        int32_t i32;
        const char *name;
    };
} Token;

Token token;
const char *stream;

const char *keyword_if;
const char *keyword_for;
const char *keyword_while;

void init_keywords(void) {
    keyword_if = str_intern("if");
    keyword_for = str_intern("for");
    keyword_while = str_intern("while");
}

void next_token(void) {
    token.lo = stream;
    switch (*stream) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        int32_t i32 = 0;
        while (isdigit(*stream)) {
            i32 = i32 * 10 + (*stream++ - '0');
        }
        token.kind = TOKEN_INT;
        token.i32 = i32;
        break;
    }
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_': {
        while (isalnum(*stream) || *stream == '_') {
            stream++;
        }
        token.kind = TOKEN_IDENT;
        token.name = str_intern_range(token.lo, stream);
        break;
    }
    default:
        token.kind = *stream++;
        break;
    }
    token.hi = stream;
}

void init_stream(const char *str) {
    stream = str;
    next_token();
}

void print_token(Token token) {
    printf("TOKEN: %d", token.kind);
    switch (token.kind) {
    case TOKEN_INT:
        printf(" %d\n", token.i32);
        break;
    case TOKEN_IDENT:
        // printf(" %.*s\n", (int)(token.hi - token.lo), token.lo);
        printf(" %s - (%p)\n", token.name, (void *)token.name);
        break;
    default:
        printf(" '%c'\n", token.kind);
        break;
    }
}

static inline bool is_token(TokenKind kind) {
    return token.kind == kind;
}

inline bool is_token_name(const char *name) {
    return token.kind == TOKEN_IDENT && token.name == name;
}

static inline bool match_token(TokenKind kind) {
    if (is_token(kind)) {
        next_token();
        return true;
    } else {
        return false;
    }
}

static inline bool expect_token(TokenKind kind) {
    if (is_token(kind)) {
        next_token();
        return true;
    } else {
        fatal("expected token: %s, got %s\n", token_kind_name(kind), token_kind_name(token.kind));
        return false;
    }
}
/*
expr3 = INT | '(' expr ')'
expr2 = [-]expr3
expr1 = expr2 ([* /] expr2)
expr0 = expr1 ([+ -] expr1)
expr = expr0
 */
int parse_expr(void);

int parse_expr3(void) {
    if (is_token(TOKEN_INT)) {
        int val = token.i32;
        next_token();
        return val;
    } else if (match_token('(')) {
        int val = parse_expr();
        expect_token(')');
        return val;
    } else {
        fatal("expected integar or '(', got '%s'\n", token_kind_name(token.kind));
        return -1;
    }
}

int parse_expr2(void) {
    if (match_token('-')) {
        return -parse_expr3();
    } else {
        return parse_expr3();
    }
}

int parse_expr1(void) {
    int val = parse_expr2();
    while (is_token('*') || is_token('/')) {
        char op = token.kind;
        next_token();
        int rval = parse_expr2();
        if (op == '*') {
            val *= rval;
        } else {
            assert(op == '/');
            assert(rval != 0);
            val /= rval;
        }
    }
    return val;
}

int parse_expr0(void) {
    int val = parse_expr1();
    while (is_token('+') || is_token('-')) {
        char op = token.kind;
        next_token();
        int rval = parse_expr1();
        if (op == '+') {
            val += rval;
        } else {
            assert(op == '-');
            val -= rval;
        }
    }
    return val;
}

int parse_expr(void) {
    return parse_expr0();
}

int test_parse_expr(const char *str) {
    init_stream(str);
    return parse_expr();
}

#define assert_expr(x) assert(test_parse_expr(#x) == (x))
void parse_test(void) {
    // clang-format off
    assert_expr(1);
    assert_expr(-1);
    assert_expr(1-2-3);
    assert_expr(2*3+4*5);
    assert_expr(2+-3);
    assert_expr(1+2-6/3+6*(3+2));
    // clang-format on
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) && match_token(TOKEN_IDENT))
#define assert_token_int(x) assert(token.i32 == (x) && match_token(TOKEN_INT))
#define assert_token_eof() assert(is_token(0))

void lex_test(void) {
    char *src = "XY+(XY)1234-_jehllo!huhu_ui,994aa12";
    init_stream(src);
    assert_token_name("XY");
    assert_token('+');
    assert_token('(');
    assert_token_name("XY");
    assert_token(')');
    assert_token_int(1234);
    assert_token('-');
    assert_token_name("_jehllo");
    assert_token('!');
    assert_token_name("huhu_ui");
    assert_token(',');
    assert_token_int(994);
    assert_token_name("aa12");
    assert_token_eof();
}

int main(void) {
    buf_test();
    lex_test();
    str_intern_test();
    parse_test();
    return 0;
}
