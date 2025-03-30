#include <assert.h>
#include <ctype.h>
#include <math.h>
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

void syntax_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("Syntax Error: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
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
    size_t num = 1024;
    size_t *vec = NULL;
    assert(buf_len(vec) == 0);
    for (size_t i = 0; i < num; i++) {
        buf_push(vec, i);
    }
    assert(num == buf_len(vec));
    for (size_t i = 0; i < num; i++) {
        assert(i == vec[i]);
    }
    buf_free(vec);
    assert(vec == NULL);
    assert(buf_len(vec) == 0);
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
    char a[] = "hello";
    assert(strcmp(a, str_intern(a)) == 0);
    assert(str_intern(a) == str_intern(a));
    assert(str_intern(str_intern(a)) == str_intern(a));
    char b[] = "hello";
    assert(a != b);
    assert(str_intern(a) == str_intern(b));
    char c[] = "hello!";
    assert(str_intern(a) != str_intern(c));
    char d[] = "hell";
    assert(str_intern(a) != str_intern(d));
}

typedef enum TokenKind {
    TOKEN_EOF = 0,
    // reserve first 128 ascii for one-char tokens
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STR,
    TOKEN_IDENT,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,
    TOKEN_EQ,
    TOKEN_NOTEQ,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_COLON_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
} TokenKind;

typedef enum TokenMod {
    TOKENMOD_NONE,
    TOKENMOD_HEX,
    TOKENMOD_BIN,
    TOKENMOD_OCT,
    TOKENMOD_CHAR,
} TokenMod;

size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind) {
    size_t n = 0;
    switch (kind) {
    case 0:
        n = snprintf(dest, dest_size, "<<EOF>>");
        break;
    case TOKEN_INT:
        n = snprintf(dest, dest_size, "integer");
        break;
    case TOKEN_FLOAT:
        n = snprintf(dest, dest_size, "float");
        break;
    case TOKEN_STR:
        n = snprintf(dest, dest_size, "string");
        break;
    case TOKEN_IDENT:
        n = snprintf(dest, dest_size, "identifier");
        break;
    default:
        if (kind < 128 && isprint(kind)) {
            n = snprintf(dest, dest_size, "%c", kind);
        } else {
            n = snprintf(dest, dest_size, "<ASCII %d>", kind);
        }
        break;
    }
    return n;
}

const char *token_kind_name(TokenKind kind) {
    static char buf[256];
    size_t n = copy_token_kind_str(buf, sizeof(buf), kind);
    assert(n + 1 <= sizeof(buf));
    return buf;
}

typedef struct Token {
    TokenKind kind;
    TokenMod mod;
    const char *lo;
    const char *hi;
    union {
        uint64_t u64;
        double f64;
        const char *str_val;
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

uint8_t char_to_digit[256] = {
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10,
    ['A'] = 10,
    ['b'] = 11,
    ['B'] = 11,
    ['c'] = 12,
    ['C'] = 12,
    ['d'] = 13,
    ['D'] = 13,
    ['e'] = 14,
    ['E'] = 14,
    ['f'] = 15,
    ['F'] = 15,
};

void scan_int(void) {
    uint64_t base = 10;
    // handle base
    if (*stream == '0') {
        stream++;

        if (tolower(*stream) == 'x') {
            stream++;
            token.mod = TOKENMOD_HEX;
            base = 16;
        } else if (tolower(*stream) == 'b') {
            stream++;
            token.mod = TOKENMOD_BIN;
            base = 2;
        } else if (isdigit(*stream)) {
            token.mod = TOKENMOD_OCT;
            base = 8;
        }
    }

    uint64_t value = 0;
    for (;;) {
        uint64_t digit = char_to_digit[(unsigned char)*stream];
        if (digit == 0 && *stream != '0')
            break;

        if (digit >= base) {
            syntax_error("digit '%c' out of range for base %ull", *stream, base);
            digit = 0;
        }

        if (value > (UINT64_MAX - digit) / base) {
            syntax_error("Integar literal overflow");
            while (isdigit(*stream))
                stream++;
            value = 0;
        }
        value = value * base + digit;
        stream++;
    }
    token.kind = TOKEN_INT;
    token.u64 = value;
}

void scan_float(void) {
    const char *start = stream;
    while (isdigit(*stream))
        stream++;

    if (*stream == '.')
        stream++;

    while (isdigit(*stream))
        stream++;

    if (tolower(*stream) == 'e') {
        stream++;
        if (*stream == '+' || *stream == '-')
            stream++;
        if (!isdigit(*stream))
            syntax_error("expected digit after float loteral exponent, found '%c'.", *stream);

        while (isdigit(*stream))
            stream++;
    }

    // const char *end = stream;
    double value = strtod(start, NULL);
    if (value == HUGE_VAL || value == -HUGE_VAL)
        syntax_error("float literal overflow");

    token.kind = TOKEN_FLOAT;
    token.f64 = value;
}

char escape_to_char[256] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['0'] = 0,
};

void scan_char(void) {
    assert(*stream == '\'');
    stream++;
    char value = 0;

    if (*stream == '\'') {
        syntax_error("char literal cannot be empty");
        stream++;
    } else if (*stream == '\n') {
        syntax_error("char literal cannot contain newlines");
    } else if (*stream == '\\') {
        stream++;
        value = escape_to_char[(unsigned char)*stream];
        if (value == 0 && *stream != '0') {
            syntax_error("invalid char literal escape '\\%c'", *stream);
        }
        stream++;
    } else {
        value = *stream;
        stream++;
    }
    if (*stream != '\'')
        syntax_error("expected closing char quote, got '%c'", *stream);
    else
        stream++;

    token.kind = TOKEN_INT;
    token.mod = TOKENMOD_CHAR;
    token.u64 = value;
}

void scan_str(void) {
    assert(*stream == '"');
    stream++;

    char *str = NULL;
    while (*stream && *stream != '"') {
        char value = *stream;
        if (value == '\n')
            syntax_error("string literal cannot contain new lines");
        else if (value == '\\') {
            stream++;
            value = escape_to_char[(unsigned char)*stream];
            if (value == 0 && *stream != '0')
                syntax_error("invalid string literal escape '\\%c'", *stream);
        }
        buf_push(str, value);
        stream++;
    }
    if (*stream) {
        assert(*stream == '"');
        stream++;
    } else
        syntax_error("unexpected eof within string litreal.");

    buf_push(str, 0);
    token.kind = TOKEN_STR;
    token.str_val = str;
}

#define CASE1(c0, c1, k)        \
    case c0:                    \
        token.kind = *stream++; \
        if (*stream == c1) {    \
            token.kind = k;     \
            stream++;           \
        }                       \
        break;

#define CASE2(c0, c1, k1, c2, k2)   \
    case c0:                        \
        token.kind = *stream++;     \
        if (*stream == c1) {        \
            token.kind = k1;        \
            stream++;               \
        } else if (*stream == c2) { \
            token.kind = k2;        \
            stream++;               \
        }                           \
        break;

void next_token(void) {
repeat:
    token.lo = stream;
    switch (*stream) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\v':
        while (isspace(*stream))
            stream++;

        goto repeat;
        break;

    case '\'':
        scan_char();
        break;

    case '"':
        scan_str();
        break;

    case '.':
        scan_float();
        break;

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
        while (isdigit(*stream))
            stream++;

        char c = *stream;
        stream = token.lo;

        if (c == '.' || tolower(c) == 'e')
            scan_float();
        else
            scan_int();
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

    case '<':
        token.kind = *stream++;
        if (*stream == '<') {
            token.kind = TOKEN_LSHIFT;
            stream++;
            if (*stream == '=') {
                token.kind = TOKEN_LSHIFT_ASSIGN;
                stream++;
            }
        } else if (*stream == '=') {
            token.kind = TOKEN_LTEQ;
            stream++;
        }
        break;

    case '>':
        token.kind = *stream++;
        if (*stream == '>') {
            token.kind = TOKEN_RSHIFT;
            stream++;
            if (*stream == '=') {
                token.kind = TOKEN_RSHIFT_ASSIGN;
                stream++;
            }
        } else if (*stream == '=') {
            token.kind = TOKEN_GTEQ;
            stream++;
        }
        break;

        CASE1('^', '=', TOKEN_XOR_ASSIGN)
        CASE1(':', '=', TOKEN_COLON_ASSIGN)
        CASE1('*', '=', TOKEN_MUL_ASSIGN)
        CASE1('/', '=', TOKEN_DIV_ASSIGN)
        CASE1('%', '=', TOKEN_MOD_ASSIGN)
        CASE2('+', '=', TOKEN_ADD_ASSIGN, '+', TOKEN_INC)
        CASE2('-', '=', TOKEN_SUB_ASSIGN, '-', TOKEN_DEC)
        CASE2('&', '=', TOKEN_AND_ASSIGN, '&', TOKEN_AND)
        CASE2('|', '=', TOKEN_OR_ASSIGN, '|', TOKEN_OR)

    default:
        token.kind = *stream++;
        break;
    }
    token.hi = stream;
}

#undef CASE1
#undef CASE2

void init_stream(const char *str) {
    stream = str;
    next_token();
}

void print_token(Token token) {
    printf("TOKEN: %d", token.kind);
    switch (token.kind) {
    case TOKEN_INT:
        printf(" %llu\n", token.u64);
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
        int val = token.u64;
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
#undef assert_expr

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) && match_token(TOKEN_IDENT))
#define assert_token_int(x) assert(token.u64 == (x) && match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.f64 == (x) && match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token.str_val, (x)) == 0 && match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(0))

void lex_test(void) {
    // Integer literal tests
    init_stream("0 18446744073709551615 0xffffffffffffffff 042 0b1111");
    assert_token_int(0);
    assert_token_int(18446744073709551615ull);
    // assert(token.mod == TOKENMOD_HEX);
    assert_token_int(0xffffffffffffffffull);
    // assert(token.mod == TOKENMOD_OCT);
    assert_token_int(042);
    // assert(token.mod == TOKENMoD_BIN);
    assert_token_int(0xF);
    assert_token_eof();

    // Float literal tests
    init_stream("3.14 .123 42. 3e10");
    assert_token_float(3.14);
    assert_token_float(.123);
    assert_token_float(42.);
    assert_token_float(3e10);
    assert_token_eof();

    // Char literal tests
    init_stream("'a' '\\n'");
    assert_token_int('a');
    assert_token_int('\n');
    assert_token_eof();

    // String literal tests
    init_stream("\"foo\" \"a\\nb\"");
    assert_token_str("foo");
    assert_token_str("a\nb");
    assert_token_eof();

    // Operator tests
    init_stream(": := + += ++ < <= << <<=");
    assert_token(':');
    assert_token(TOKEN_COLON_ASSIGN);
    assert_token('+');
    assert_token(TOKEN_ADD_ASSIGN);
    assert_token(TOKEN_INC);
    assert_token('<');
    assert_token(TOKEN_LTEQ);
    assert_token(TOKEN_LSHIFT);
    assert_token(TOKEN_LSHIFT_ASSIGN);
    assert_token_eof();

    init_stream("XY+(XY)1234-_jehllo!huhu_ui,994 aa12");
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

#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_float
#undef assert_token_str
#undef assert_token_eof

void run_tests(void) {
    buf_test();
    lex_test();
    str_intern_test();
}

int main(void) {
    run_tests();
    return 0;
}
