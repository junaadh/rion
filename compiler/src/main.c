#include <assert.h>
#include <ctype.h>
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

#define GROWTH_FACTOR 2
typedef struct BufHdr {
  size_t len;
  size_t cap;
  char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n)                                                         \
  (buf__fits(b, n) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, x)                                                         \
  (buf__fit(b, 1), (b)[buf_len(b)] = (x), buf__hdr(b)->len++)
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

typedef struct InternStr {
  size_t len;
  const char *str;
} InternStr;

static InternStr *interns;

const char *str_intern_range(const char *start, const char *end) {
  size_t len = end - start;
  for (size_t i = 0; i < buf_len(interns); i++) {
    if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0) {
      return interns[i].str;
    }
  }
  char *str = xmalloc(len + 1);
  memcpy(str, start, len);
  str[len] = 0;
  buf_push(interns, ((InternStr){len, str}));
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
  const char* pz = str_intern(z);
  assert(px != pz);
}

typedef enum TokenKind {
  TOKEN_INT = 128,
  TOKEN_IDENT,
  // ...
} TokenKind;

typedef struct Token {
  TokenKind kind;
  const char *lo;
  const char *hi;
  union {
    uint64_t u64;
  };
} Token;

Token token;
const char *stream;

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
    uint64_t u64 = 0;
    while (isdigit(*stream)) {
      u64 = u64 * 10 + (*stream++ - '0');
    }
    token.kind = TOKEN_INT;
    token.u64 = u64;
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
    break;
  }
  default:
    token.kind = *stream++;
    break;
  }
  token.hi = stream;
}

void print_token(Token token) {
  printf("TOKEN: %d", token.kind);
  switch (token.kind) {
  case TOKEN_INT:
    printf(" %llu\n", token.u64);
    break;
  case TOKEN_IDENT:
    printf(" %.*s\n", (int)(token.hi - token.lo), token.lo);
    break;
  default:
    printf(" '%c'\n", token.kind);
    break;
  }
}

void lex_test(void) {
  char *src = "+()1234-_jehllo!huhu_ui,994aa12";
  stream = src;
  next_token();
  while (token.kind) {
    print_token(token);
    next_token();
  }
}

int main(void) {
  buf_test();
  lex_test();
  str_intern_test();
  return 0;
}
