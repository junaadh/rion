#include "ast.h"

Arena ast_arena;

void *ast_alloc(size_t size) {
    assert(size != 0);
    void *ptr = arena_alloc(&ast_arena, size);
    memset(ptr, 0, size);
    return ptr;
}

void *ast_dup(const void *src, size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = arena_alloc(&ast_arena, size);
    memcpy(ptr, src, size);
    return ptr;
}

Typespec *typespec_new(TypespecKind kind) {
    Typespec *t = ast_alloc(sizeof(Typespec));
    t->kind = kind;
    return t;
}

Typespec *typespec_ident(const char *name) {
    Typespec *t = typespec_new(TYPESPEC_IDENT);
    t->name = name;
    return t;
}

Typespec *typespec_ptr(Typespec *elem) {
    Typespec *t = typespec_new(TYPESPEC_PTR);
    t->ptr.elem = elem;
    return t;
}

Typespec *typespec_array(Typespec *elem, Expr *size) {
    Typespec *t = typespec_new(TYPESPEC_ARRAY);
    t->array.elem = elem;
    t->array.size = size;
    return t;
}

Typespec *typespec_fn(Typespec **args, size_t num_args, Typespec *ret) {
    Typespec *t = typespec_new(TYPESPEC_FN);
    t->fn.args = args;
    t->fn.num_args = num_args;
    t->fn.ret = ret;
    return t;
}

Decl *decl_new(DeclKind kind, const char *name) {
    Decl *d = ast_alloc(sizeof(Decl));
    d->kind = kind;
    d->name = name;
    return d;
}

Decl *decl_enum(const char *name, EnumItem *items, size_t num_items) {
    Decl *d = decl_new(DECL_ENUM, name);
    d->enum_decl.items = items;
    d->enum_decl.num_items = num_items;
    return d;
}

Decl *decl_aggregate(DeclKind kind, const char *name, AggregateItem *items, size_t num_items) {
    assert(kind == DECL_STRUCT || kind == DECL_UNION);
    Decl *d = decl_new(kind, name);
    d->aggregate.items = items;
    d->aggregate.num_items = num_items;
    return d;
}

Decl *decl_union(const char *name, AggregateItem *items, size_t num_items) {
    Decl *d = decl_new(DECL_UNION, name);
    d->aggregate.items = items;
    d->aggregate.num_items = num_items;
    return d;
}

Decl *decl_let(const char *name, Typespec *type, Expr *expr) {
    Decl *d = decl_new(DECL_LET, name);
    d->let.type = type;
    d->let.expr = expr;
    return d;
}

Decl *decl_fn(const char *name, FnParam *params, size_t num_params, Typespec *ret_type, StmtBlock block) {
    Decl *d = decl_new(DECL_FN, name);
    d->fn.params = params;
    d->fn.num_params = num_params;
    d->fn.ret_type = ret_type;
    d->fn.block = block;
    return d;
}

Decl *decl_const(const char *name, Expr *expr) {
    Decl *d = decl_new(DECL_CONST, name);
    d->const_decl.expr = expr;
    return d;
}

Decl *decl_typedef(const char *name, Typespec *type) {
    Decl *d = decl_new(DECL_TYPEDEF, name);
    d->typedef_decl.type = type;
    return d;
}

Expr *expr_new(ExprKind kind) {
    Expr *e = ast_alloc(sizeof(Expr));
    e->kind = kind;
    return e;
}

Expr *expr_int(uint64_t value) {
    Expr *e = expr_new(EXPR_INT);
    e->int_val = value;
    return e;
}

Expr *expr_float(double float_val) {
    Expr *e = expr_new(EXPR_FLOAT);
    e->float_val = float_val;
    return e;
}

Expr *expr_str(const char *str_val) {
    Expr *e = expr_new(EXPR_STR);
    e->str_val = str_val;
    return e;
}

Expr *expr_ident(const char *name) {
    Expr *e = expr_new(EXPR_IDENT);
    e->name = name;
    return e;
}

Expr *expr_compound(Typespec *type, Expr **args, size_t num_args) {
    Expr *e = expr_new(EXPR_COMPOUND);
    e->compound.type = type;
    e->compound.args = args;
    e->compound.num_args = num_args;
    return e;
}

Expr *expr_cast(Typespec *type, Expr *expr) {
    Expr *e = expr_new(EXPR_CAST);
    e->cast.type = type;
    e->cast.expr = expr;
    return e;
}

Expr *expr_call(Expr *expr, Expr **args, size_t num_args) {
    Expr *e = expr_new(EXPR_CALL);
    e->call.expr = expr;
    e->call.args = args;
    e->call.num_args = num_args;
    return e;
}

Expr *expr_index(Expr *expr, Expr *index) {
    Expr *e = expr_new(EXPR_INDEX);
    e->index.expr = expr;
    e->index.index = index;
    return e;
}

Expr *expr_field(Expr *expr, const char *name) {
    Expr *e = expr_new(EXPR_FIELD);
    e->field.expr = expr;
    e->field.name = name;
    return e;
}

Expr *expr_unary(TokenKind op, Expr *expr) {
    Expr *e = expr_new(EXPR_UNARY);
    e->unary.op = op;
    e->unary.expr = expr;
    return e;
}

Expr *expr_binary(TokenKind op, Expr *left, Expr *right) {
    Expr *e = expr_new(EXPR_BINARY);
    e->binary.op = op;
    e->binary.left = left;
    e->binary.right = right;
    return e;
}

Expr *expr_ternary(Expr *cond, Expr *then_expr, Expr *else_expr) {
    Expr *e = expr_new(EXPR_TERNARY);
    e->ternary.cond = cond;
    e->ternary.then_expr = then_expr;
    e->ternary.else_expr = else_expr;
    return e;
}

Stmt *stmt_new(StmtKind kind) {
    Stmt *s = ast_alloc(sizeof(Stmt));
    s->kind = kind;
    return s;
}

Stmt *stmt_return(Expr *expr) {
    Stmt *s = stmt_new(STMT_RETURN);
    s->return_stmt.expr = expr;
    return s;
}

Stmt *stmt_break(void) {
    return stmt_new(STMT_BREAK);
}

Stmt *stmt_continue(void) {
    return stmt_new(STMT_CONTINUE);
}

Stmt *stmt_block(StmtBlock block) {
    Stmt *s = stmt_new(STMT_BLOCK);
    s->block = block;
    return s;
}

Stmt *stmt_if(Expr *cond, StmtBlock then_block, ElseIf *elseifs, size_t num_elseifs, StmtBlock else_block) {
    Stmt *s = stmt_new(STMT_IF);
    s->if_stmt.cond = cond;
    s->if_stmt.then_block = then_block;
    s->if_stmt.elseifs = elseifs;
    s->if_stmt.num_elseifs = num_elseifs;
    s->if_stmt.else_block = else_block;
    return s;
}

Stmt *stmt_while(Expr *cond, StmtBlock block) {
    Stmt *s = stmt_new(STMT_WHILE);
    s->while_stmt.cond = cond;
    s->while_stmt.block = block;
    return s;
}

Stmt *stmt_do_while(Expr *cond, StmtBlock block) {
    Stmt *s = stmt_new(STMT_DO_WHILE);
    s->while_stmt.cond = cond;
    s->while_stmt.block = block;
    return s;
}

Stmt *stmt_for(Stmt *init, Expr *cond, Stmt *next, StmtBlock block) {
    Stmt *s = stmt_new(STMT_FOR);
    s->for_stmt.init = init;
    s->for_stmt.cond = cond;
    s->for_stmt.next = next;
    s->for_stmt.block = block;
    return s;
}

Stmt *stmt_switch(Expr *expr, SwitchCase *cases, size_t num_cases) {
    Stmt *s = stmt_new(STMT_SWITCH);
    s->switch_stmt.expr = expr;
    s->switch_stmt.cases = cases;
    s->switch_stmt.num_cases = num_cases;
    return s;
}

Stmt *stmt_assign(TokenKind op, Expr *left, Expr *right) {
    Stmt *s = stmt_new(STMT_ASSIGN);
    s->assign.op = op;
    s->assign.left = left;
    s->assign.right = right;
    return s;
}

Stmt *stmt_init(const char *name, Expr *expr) {
    Stmt *s = stmt_new(STMT_INIT);
    s->init.name = name;
    s->init.expr = expr;
    return s;
}

Stmt *stmt_expr(Expr *expr) {
    Stmt *s = stmt_new(STMT_EXPR);
    s->expr = expr;
    return s;
}
