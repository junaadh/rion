#pragma once

#include "common.h"

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

size_t copy_token_kind_str(char *dest, size_t dest_size, TokenKind kind);
const char *token_kind_name(TokenKind kind);

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

void init_keywords(void);

void scan_int(void);
void scan_float(void);
void scan_char(void);
void scan_str(void);
void next_token(void);

void init_stream(const char *str);

void print_token(Token token);
static inline bool is_token(TokenKind kind);
inline bool is_token_name(const char *name);
static inline bool match_token(TokenKind kind);
// static inline bool expect_token(TokenKind kind);

void lex_test(void);
