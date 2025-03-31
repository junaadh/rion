#pragma once

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
#define ALIGN_DOWN(n, a) ((n) & ~((a)-1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a)-1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

void *xrealloc(void *ptr, size_t num_bytes);
void *xmalloc(size_t num_bytes);

void fatal(const char *fmt, ...);
void syntax_error(const char *fmt, ...);
void fatal_syntax_error(const char *fmt, ...);

#define GROWTH_FACTOR 2

typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b) * sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_fit(b, n) \
    ((n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...) \
    (buf_fit((b), 1 + buf_len(b)), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))

void *buf__grow(const void *buf, size_t new_len, size_t elem_size);

typedef struct Arena {
    char *ptr;
    char *end;
    char **blocks;
} Arena;

#define ARENA_ALIGNMENT 8
#define ARENA_BLOCK_SIZE 1024

void arena_grow(Arena *arena, size_t min_size);
void *arena_alloc(Arena *arena, size_t size);
void arena_free(Arena *arena);

typedef struct Intern {
    size_t len;
    const char *str;
} Intern;

Intern *interns;
Arena str_arena;

const char *str_intern_range(const char *start, const char *end);
const char *str_intern(const char *str);

void common_test(void);
