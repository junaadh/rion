#include "lex.h"

#define KW(name)                        \
    name##_keyword = str_intern(#name); \
    buf_push(keywords, name##_keyword)

void init_keywords(void) {
    static bool inited;
    if (inited) {
        return;
    }
    char *arena_end = str_arena.end;
    KW(typedef);
    KW(enum);
    KW(struct);
    KW(union);
    KW(const);
    KW(let);
    KW(fn);
    KW(sizeof);
    KW(break);
    KW(continue);
    KW(return);
    KW(if);
    KW(else);
    KW(while);
    KW(do);
    KW(for);
    KW(switch);
    KW(case);
    KW(default);
    assert(str_arena.end == arena_end);
    first_keyword = typedef_keyword;
    last_keyword = default_keyword;
    inited = true;
}

#undef KW

bool is_keyword_str(const char *str) {
    // printf("str=%s, first_kw=%s, last_kw=%s, fkw<=str=%d, str<=lkw=%d\n", str, first_keyword, last_keyword, first_keyword <= str, str <= last_keyword);
    return first_keyword <= str && str <= last_keyword;
}

bool is_keyword(const char *name) {
    return is_token(TOKEN_KEYWORD) && token.name == name;
}
bool match_keyword(const char *name) {
    if (is_keyword(name)) {
        next_token();
        return true;
    } else {
        return false;
    }
}

const char *token_kind_names[] = {
    [TOKEN_EOF] = "EOF",
    [TOKEN_INT] = "int",
    [TOKEN_FLOAT] = "float",
    [TOKEN_STR] = "string",
    [TOKEN_IDENT] = "ident",
    [TOKEN_LSHIFT] = "<<",
    [TOKEN_RSHIFT] = ">>",
    [TOKEN_EQ] = "==",
    [TOKEN_NOTEQ] = "!=",
    [TOKEN_LTEQ] = "<=",
    [TOKEN_GTEQ] = ">=",
    [TOKEN_AND] = "&&",
    [TOKEN_OR] = "||",
    [TOKEN_INC] = "++",
    [TOKEN_DEC] = "--",
    [TOKEN_COLON_ASSIGN] = ":=",
    [TOKEN_ADD_ASSIGN] = "+=",
    [TOKEN_SUB_ASSIGN] = "-=",
    [TOKEN_OR_ASSIGN] = "|=",
    [TOKEN_AND_ASSIGN] = "&=",
    [TOKEN_XOR_ASSIGN] = "^=",
    [TOKEN_MUL_ASSIGN] = "+=",
    [TOKEN_DIV_ASSIGN] = "/=",
    [TOKEN_MOD_ASSIGN] = "%=",
    [TOKEN_LSHIFT_ASSIGN] = "<<=",
    [TOKEN_RSHIFT_ASSIGN] = ">>=",
};

const char *token_kind_name(TokenKind kind) {
    if (kind < sizeof(token_kind_names) / sizeof(*token_kind_names))
        return token_kind_names[kind];
    else
        return NULL;
}

size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind) {
    size_t n = 0;
    const char *name = token_kind_name(kind);
    if (name) {
        n = snprintf(dest, dest_size, "%s", name);
    } else if (kind < 128 && isprint(kind)) {
        n = snprintf(dest, dest_size, "%c", kind);
    } else {
        n = snprintf(dest, dest_size, "<ASCII %d>", kind);
    }
    return n;
}

const char *temp_token_kind_str(TokenKind kind) {
    static char buf[256];
    size_t n = copy_token_kind_str(buf, sizeof(buf), kind);
    assert(n + 1 <= sizeof(buf));
    return buf;
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
        token.name = str_intern_range(token.lo, stream);
        token.kind = is_keyword_str(token.name) ? TOKEN_KEYWORD : TOKEN_IDENT;
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

        CASE1('=', '=', TOKEN_EQ)
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

inline bool is_token(TokenKind kind) {
    return token.kind == kind;
}

inline bool is_token_name(const char *name) {
    return token.kind == TOKEN_IDENT && token.name == name;
}

inline bool match_token(TokenKind kind) {
    if (is_token(kind)) {
        next_token();
        return true;
    } else {
        return false;
    }
}

inline bool expect_token(TokenKind kind) {
    if (is_token(kind)) {
        next_token();
        return true;
    } else {
        char buf[256];
        copy_token_kind_str(buf, sizeof(buf), kind);
        fatal("expected token: %s, got %s\n", buf, temp_token_kind_str(token.kind));
        return false;
    }
}

void keyword_test(void) {
    init_keywords();
    assert(is_keyword_str(first_keyword));
    assert(is_keyword_str(last_keyword));
    for (const char **it = keywords; it != buf_end(keywords); it++) {
        assert(is_keyword_str(*it));
    }
    assert(!is_keyword_str(str_intern("foo")));
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) && match_token(TOKEN_IDENT))
#define assert_token_int(x) assert(token.u64 == (x) && match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.f64 == (x) && match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token.str_val, (x)) == 0 && match_token(TOKEN_STR))
#define assert_token_eof() assert(is_token(0))

void lex_test(void) {
    keyword_test();

    // Integer literal tests
    init_stream("0 18446744073709551615 0xffffffffffffffff 042 0b1111");
    assert_token_int(0);
    assert_token_int(18446744073709551615ull);
    assert(token.mod == TOKENMOD_HEX);
    assert_token_int(0xffffffffffffffffull);
    assert(token.mod == TOKENMOD_OCT);
    assert_token_int(042);
    assert(token.mod == TOKENMOD_BIN);
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
