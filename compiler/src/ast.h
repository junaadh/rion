#pragma once

#include "common.h"
#include "lex.h"

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Decl Decl;
typedef struct Typespec Typespec;

void *ast_alloc(size_t size);
void *ast_dup(const void *src, size_t size);

typedef struct StmtBlock {
    Stmt **stmts;
    size_t num_stmts;
} StmtBlock;

typedef enum TypespecKind {
    TYPESPEC_NONE,
    TYPESPEC_IDENT,
    TYPESPEC_FN,
    TYPESPEC_ARRAY,
    TYPESPEC_PTR,
} TypespecKind;

typedef struct FnTypespec {
    size_t num_args;
    Typespec **args;
    Typespec *ret;
} FnTypespec;

typedef struct PtrTypespec {
    Typespec *elem;
} PtrTypespec;

typedef struct ArrayTypespec {
    Typespec *elem;
    Expr *size;
} ArrayTypespec;

struct Typespec {
    TypespecKind kind;
    struct {
        const char *name;
        FnTypespec fn;
        ArrayTypespec array;
        PtrTypespec ptr;
    };
};

Typespec *typespec_new(TypespecKind kind);
Typespec *typespec_ident(const char *name);
Typespec *typespec_ptr(Typespec *elem);
Typespec *typespec_array(Typespec *elem, Expr *size);
Typespec *typespec_fn(Typespec **args, size_t num_args, Typespec *ret);

typedef enum DeclKind {
    DECL_NONE,
    DECL_ENUM,
    DECL_STRUCT,
    DECL_UNION,
    DECL_LET,
    DECL_CONST,
    DECL_TYPEDEF,
    DECL_FN,
} DeclKind;

typedef struct EnumItem {
    const char *name;
    Expr *expr;
} EnumItem;

typedef struct EnumDecl {
    EnumItem *items;
    size_t num_items;
} EnumDecl;

typedef struct AggregateItem {
    const char **names;
    size_t num_names;
    Typespec *types;
} AggregateItem;

typedef struct AggregateDecl {
    size_t num_items;
    AggregateItem *items;
} AggregateDecl;

typedef struct FnParam {
    const char *name;
    Typespec *type;
} FnParam;

typedef struct FnDecl {
    FnParam *params;
    size_t num_params;
    Typespec *ret_type;
    StmtBlock block;
} FnDecl;

typedef struct TypedefDecl {
    Typespec *type;
} TypedefDecl;

typedef struct LetDecl {
    Typespec *type;
    Expr *expr;
} LetDecl;

typedef struct ConstDecl {
    Expr *expr;
} ConstDecl;

struct Decl {
    DeclKind kind;
    const char *name;
    union {
        EnumDecl enum_decl;
        AggregateDecl aggregate;
        TypedefDecl typedef_decl;
        LetDecl let;
        ConstDecl const_decl;
        FnDecl fn;
    };
};

Decl *decl_new(DeclKind kind, const char *name);
Decl *decl_enum(const char *name, EnumItem *items, size_t num_items);
Decl *decl_aggregate(DeclKind kind, const char *name, AggregateItem *items, size_t num_items);
Decl *decl_union(const char *name, AggregateItem *items, size_t num_items);
Decl *decl_let(const char *name, Typespec *type, Expr *expr);
Decl *decl_fn(const char *name, FnParam *params, size_t num_params, Typespec *ret_type, StmtBlock block);
Decl *decl_const(const char *name, Expr *expr);
Decl *decl_typedef(const char *name, Typespec *type);

typedef enum ExprKind {
    EXPR_NONE,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STR,
    EXPR_IDENT,
    EXPR_CAST,
    EXPR_CALL,
    EXPR_INDEX,
    EXPR_FIELD,
    EXPR_COMPOUND,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
} ExprKind;

typedef struct CompoundExpr {
    Typespec *type;
    Expr **args;
    size_t num_args;
} CompoundExpr;

typedef struct CastExpr {
    Typespec *type;
    Expr *expr;
} CastExpr;

typedef struct UnaryExpr {
    TokenKind op;
    Expr *expr;
} UnaryExpr;

typedef struct BinaryExpr {
    TokenKind op;
    Expr *left;
    Expr *right;
} BinaryExpr;

typedef struct TernaryExpr {
    Expr *cond;
    Expr *then_expr;
    Expr *else_expr;
} TernaryExpr;

typedef struct CallExpr {
    Expr *expr;
    Expr **args;
    size_t num_args;
} CallExpr;

typedef struct IndexExpr {
    Expr *expr;
    Expr *index;
} IndexExpr;

typedef struct FieldExpr {
    Expr *expr;
    const char *name;
} FieldExpr;

struct Expr {
    ExprKind kind;
    union {
        uint64_t int_val;
        double float_val;
        const char *str_val;
        const char *name;
        CompoundExpr compound;
        CastExpr cast;
        UnaryExpr unary;
        BinaryExpr binary;
        TernaryExpr ternary;
        CallExpr call;
        IndexExpr index;
        FieldExpr field;
    };
};

Expr *expr_new(ExprKind kind);
Expr *expr_int(uint64_t value);
Expr *expr_float(double value);
Expr *expr_str(const char *str);
Expr *expr_ident(const char *ident);
Expr *expr_compound(Typespec *type, Expr **args, size_t num_args);
Expr *expr_cast(Typespec *type, Expr *expr);
Expr *expr_call(Expr *expr, Expr **args, size_t num_args);
Expr *expr_index(Expr *expr, Expr *index);
Expr *expr_field(Expr *expr, const char *name);
Expr *expr_unary(TokenKind op, Expr *expr);
Expr *expr_binary(TokenKind op, Expr *left, Expr *right);
Expr *expr_ternary(Expr *cond, Expr *then_expr, Expr *else_expr);

typedef enum StmtKind {
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_SWITCH,
    STMT_ASSIGN,
    STMT_INIT,
    STMT_EXPR,
} StmtKind;

typedef struct ReturnStmt {
    Expr *expr;
} ReturnStmt;

typedef struct ElseIf {
    Expr *cond;
    StmtBlock block;
} ElseIf;

typedef struct IfStmt {
    Expr *cond;
    StmtBlock then_block;
    ElseIf *elseifs;
    size_t num_elseifs;
    StmtBlock else_block;
} IfStmt;

typedef struct WhileStmt {
    Expr *cond;
    StmtBlock block;
} WhileStmt;

typedef struct ForStmt {
    Stmt *init;
    Expr *cond;
    Stmt *next;
    StmtBlock block;
} ForStmt;

typedef struct SwitchCase {
    Expr **exprs;
    size_t num_exprs;
    bool is_default;
    StmtBlock block;
} SwitchCase;

typedef struct SwitchStmt {
    Expr *expr;
    SwitchCase *cases;
    size_t num_cases;
} SwitchStmt;

typedef struct AssignStmt {
    TokenKind op;
    Expr *left;
    Expr *right;
} AssignStmt;

typedef struct InitStmt {
    const char *name;
    Expr *expr;
} InitStmt;

struct Stmt {
    StmtKind kind;
    union {
        ReturnStmt return_stmt;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        SwitchStmt switch_stmt;
        StmtBlock block;
        AssignStmt assign;
        InitStmt init;
        Expr *expr;
    };
};

Stmt *stmt_new(StmtKind kind);
Stmt *stmt_return(Expr *expr);
Stmt *stmt_break(void);
Stmt *stmt_continue(void);
Stmt *stmt_block(StmtBlock block);
Stmt *stmt_if(Expr *cond, StmtBlock then_block, ElseIf *elseifs, size_t num_elseifs, StmtBlock else_block);
Stmt *stmt_while(Expr *cond, StmtBlock block);
Stmt *stmt_do_while(Expr *cond, StmtBlock block);
Stmt *stmt_for(Stmt *init, Expr *cond, Stmt *next, StmtBlock block);
Stmt *stmt_switch(Expr *expr, SwitchCase *cases, size_t num_cases);
Stmt *stmt_assign(TokenKind op, Expr *left, Expr *right);
Stmt *stmt_init(const char *name, Expr *expr);
Stmt *stmt_expr(Expr *expr);

/*
 * print.c
 */

void print_expr(Expr *expr);
void print_stmt(Stmt *stmt);
void print_decl(Decl *decl);
void print_test(void);

/*
 * parse.c
 */

Typespec *parse_type_fn(void);
Typespec *parse_type_base(void);
Typespec *parse_type(void);
Expr *parse_expr_compound(Typespec *type);
Expr *parse_expr_operand(void);
Expr *parse_expr_base(void);
bool is_unary_op(void);
bool is_mul_op(void);
bool is_add_op(void);
bool is_cmp_op(void);
bool is_assign_op(void);
Expr *parse_expr_unary(void);
Expr *parse_expr_mul(void);
Expr *parse_expr_add(void);
Expr *parse_expr_cmp(void);
Expr *parse_expr_and(void);
Expr *parse_expr_or(void);
Expr *parse_expr_ternary(void);
Expr *parse_expr(void);
Expr *parse_paren_expr(void);
StmtBlock parse_stmt_block(void);
Stmt *parse_stmt_if(void);
Stmt *parse_stmt_while(void);
Stmt *parse_stmt_do_while(void);
Stmt *parse_simple_stmt(void);
Stmt *parse_stmt_for(void);
SwitchCase parse_stmt_switch_case(void);
Stmt *parse_stmt_switch(void);
Stmt *parse_stmt(void);
const char *parse_ident(void);
Decl *parse_decl_enum(void);
AggregateItem parse_decl_aggregate_item(void);
Decl *parse_decl_aggregate(DeclKind kind);
Decl *parse_decl_let(void);
Decl *parse_decl_const(void);
Decl *parse_decl_typedef(void);
FnParam parse_decl_fn_param(void);
Decl *parse_decl_fn(void);
Decl *parse_decl(void);
void parse_and_print_decl(const char *str);
void parse_test(void);
