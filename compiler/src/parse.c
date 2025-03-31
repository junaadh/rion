#include "ast.h"
#include "common.h"
#include "lex.h"
#include <stdio.h>

Typespec *parse_type_fn(void) {
    Typespec **args = NULL;
    expect_token('(');
    if (!is_token(')')) {
        buf_push(args, parse_type());
        while (match_token(',')) {
            buf_push(args, parse_type());
        }
    }
    expect_token(')');
    Typespec *ret = NULL;
    if (match_token(':')) {
        ret = parse_type();
    }
    return typespec_fn(ast_dup(args, buf_sizeof(args)), buf_len(args), ret);
}

Typespec *parse_type_base(void) {
    if (is_token(TOKEN_IDENT)) {
        const char *name = token.name;
        next_token();
        return typespec_ident(name);
    } else if (match_keyword(fn_keyword)) {
        return parse_type_fn();
    } else if (match_token('(')) {
        return parse_type();
    } else {
        fatal_syntax_error("Unexpected token %s in type", temp_token_kind_str(token.kind));
        return NULL;
    }
}

Typespec *parse_type(void) {
    Typespec *type = parse_type_base();
    while (is_token('[') || is_token('*')) {
        if (match_token('[')) {
            Expr *expr = NULL;
            if (!is_token(']')) {
                expr = parse_expr();
            }
            expect_token(']');
            type = typespec_array(type, expr);
        } else {
            assert(is_token('*'));
            next_token();
            type = typespec_ptr(type);
        }
    }
    return type;
}

Expr *parse_expr_compound(Typespec *type) {
    expect_token('{');
    Expr **args = NULL;
    if (!is_token('}')) {
        buf_push(args, parse_expr());
        while (match_token(',')) {
            buf_push(args, parse_expr());
        }
    }
    expect_token('}');
    return expr_compound(type, ast_dup(args, buf_sizeof(args)), buf_len(args));
}

Expr *parse_expr_operand(void) {
    if (is_token(TOKEN_INT)) {
        uint64_t value = token.u64;
        next_token();
        return expr_int(value);
    } else if (is_token(TOKEN_FLOAT)) {
        double value = token.f64;
        next_token();
        return expr_float(value);
    } else if (is_token(TOKEN_STR)) {
        const char *val = token.str_val;
        next_token();
        return expr_str(val);
    } else if (is_token(TOKEN_IDENT)) {
        const char *name = token.name;
        next_token();
        if (is_token('{')) {
            return parse_expr_compound(typespec_ident(name));
        } else {
            return expr_ident(name);
        }
    } else if (is_token('{')) {
        return parse_expr_compound(NULL);
    } else if (is_token('(')) {
        if (is_token(':')) {
            Typespec *type = parse_type();
            expect_token(')');
            return parse_expr_compound(type);
        } else {
            Expr *expr = parse_expr();
            expect_token(')');
            return expr;
        }
    } else {
        fatal_syntax_error("Unexpected token %s in expression", temp_token_kind_str(token.kind));
        return NULL;
    }
}

Expr *parse_expr_base(void) {
    Expr *expr = parse_expr_operand();
    while (is_token('(') || is_token('[') || is_token('.')) {
        if (match_token('(')) {
            Expr **args = NULL;
            if (!is_token(')')) {
                buf_push(args, parse_expr());
                while (match_token(',')) {
                    buf_push(args, parse_expr());
                }
            }
            expect_token(')');
            expr = expr_call(expr, ast_dup(args, buf_sizeof(args)), buf_len(args));
        } else if (match_token('[')) {
            Expr *index = parse_expr();
            expect_token(']');
            expr = expr_index(expr, index);
        } else {
            assert(is_token('.'));
            next_token();
            const char *field = token.name;
            expect_token(TOKEN_IDENT);
            expr = expr_field(expr, field);
        }
    }
    return expr;
}

bool is_unary_op(void) {
    return is_token('+') || is_token('-') || is_token('*') || is_token('&');
}

bool is_mul_op(void) {
    return is_token('*') || is_token('/') || is_token('%') || is_token('&') || is_token(TOKEN_LSHIFT) || is_token(TOKEN_RSHIFT);
}

bool is_add_op(void) {
    return is_token('+') || is_token('-') || is_token('|') || is_token('^');
}

bool is_cmp_op(void) {
    return is_token('<') || is_token('>') || is_token(TOKEN_EQ) || is_token(TOKEN_NOTEQ) || is_token(TOKEN_GTEQ) || is_token(TOKEN_LTEQ);
}

bool is_assign_op(void) {
    return TOKEN_FIRST_ASSIGN <= token.kind && token.kind <= TOKEN_LAST_ASSIGN;
}

Expr *parse_expr_unary(void) {
    if (is_unary_op()) {
        TokenKind op = token.kind;
        next_token();
        return expr_unary(op, parse_expr_unary());
    } else {
        return parse_expr_base();
    }
}

Expr *parse_expr_mul(void) {
    Expr *expr = parse_expr_unary();
    while (is_mul_op()) {
        TokenKind op = token.kind;
        next_token();
        expr = expr_binary(op, expr, parse_expr_unary());
    }
    return expr;
}

Expr *parse_expr_add(void) {
    Expr *expr = parse_expr_mul();
    while (is_add_op()) {
        TokenKind op = token.kind;
        next_token();
        expr = expr_binary(op, expr, parse_expr_mul());
    }
    return expr;
}

Expr *parse_expr_cmp(void) {
    Expr *expr = parse_expr_add();
    while (is_cmp_op()) {
        TokenKind op = token.kind;
        next_token();
        expr = expr_binary(op, expr, parse_expr_add());
    }
    return expr;
}

Expr *parse_expr_and(void) {
    Expr *expr = parse_expr_cmp();
    while (match_token(TOKEN_AND)) {
        expr = expr_binary(TOKEN_AND, expr, parse_expr_cmp());
    }
    return expr;
}

Expr *parse_expr_or(void) {
    Expr *expr = parse_expr_and();
    while (match_token(TOKEN_OR)) {
        expr = expr_binary(TOKEN_OR, expr, parse_expr_and());
    }
    return expr;
}

Expr *parse_expr_ternary(void) {
    Expr *expr = parse_expr_or();
    if (match_token('?')) {
        Expr *then_expr = parse_expr_ternary();
        expect_token(':');
        Expr *else_expr = parse_expr_ternary();
        expr = expr_ternary(expr, then_expr, else_expr);
    }
    return expr;
}

Expr *parse_expr(void) {
    return parse_expr_ternary();
}

Expr *parse_paren_expr(void) {
    expect_token('(');
    Expr *expr = parse_expr();
    expect_token(')');
    return expr;
}

StmtBlock parse_stmt_block(void) {
    expect_token('{');
    Stmt **stmts = NULL;
    while (!is_token(TOKEN_EOF) && !is_token('}')) {
        buf_push(stmts, parse_stmt());
    }
    expect_token('}');
    return (StmtBlock){ast_dup(stmts, buf_sizeof(stmts)), buf_len(stmts)};
}

Stmt *parse_stmt_if(void) {
    Expr *cond = parse_paren_expr();
    StmtBlock then_block = parse_stmt_block();
    StmtBlock else_block = {0};
    ElseIf *elseifs = NULL;
    while (match_keyword(else_keyword)) {
        if (!match_keyword(if_keyword)) {
            else_block = parse_stmt_block();
            break;
        }
        Expr *elseif_cond = parse_paren_expr();
        StmtBlock elseif_block = parse_stmt_block();
        buf_push(elseifs, (ElseIf){elseif_cond, elseif_block});
    }
    return stmt_if(cond, then_block, ast_dup(elseifs, buf_sizeof(elseifs)), buf_len(elseifs), else_block);
}

Stmt *parse_stmt_while(void) {
    Expr *cond = parse_paren_expr();
    return stmt_while(cond, parse_stmt_block());
}

Stmt *parse_stmt_do_while(void) {
    StmtBlock block = parse_stmt_block();
    if (!match_keyword(while_keyword)) {
        fatal_syntax_error("Expected 'while' after 'do' block");
        return NULL;
    }
    Expr *cond = parse_paren_expr();
    Stmt *stmt = stmt_do_while(cond, block);
    expect_token(';');
    return stmt;
}

Stmt *parse_simple_stmt(void) {
    Expr *expr = parse_expr();
    Stmt *stmt;
    if (match_token(TOKEN_COLON_ASSIGN)) {
        if (expr->kind != EXPR_IDENT) {
            fatal_syntax_error(":= must be preceded by a name");
        }
        stmt = stmt_init(expr->name, parse_expr());
    } else if (is_assign_op()) {
        TokenKind op = token.kind;
        next_token();
        stmt = stmt_assign(op, expr, parse_expr());
    } else if (is_token(TOKEN_INC) || is_token(TOKEN_DEC)) {
        TokenKind op = token.kind;
        next_token();
        stmt = stmt_assign(op, expr, NULL);
    } else {
        stmt = stmt_expr(expr);
    }
    return stmt;
}

Stmt *parse_stmt_for(void) {
    expect_token('(');
    Stmt *init = NULL;
    if (!is_token(';')) {
        init = parse_simple_stmt();
    }
    expect_token(';');
    Expr *cond = NULL;
    if (!is_token(';')) {
        cond = parse_expr();
    }
    expect_token(';');
    Stmt *next = NULL;
    if (!is_token(')')) {
        next = parse_simple_stmt();
    }
    expect_token(')');
    return stmt_for(init, cond, next, parse_stmt_block());
}

SwitchCase parse_stmt_switch_case(void) {
    Expr **exprs = NULL;
    bool is_default = false;
    while (is_keyword(case_keyword) || is_keyword(default_keyword)) {
        if (match_keyword(case_keyword)) {
            buf_push(exprs, parse_expr());
            expect_token(':');
        } else {
            assert(is_keyword(default_keyword));
            next_token();
            is_default = true;
        }
    }
    StmtBlock block = parse_stmt_block();
    return (SwitchCase){ast_dup(exprs, buf_sizeof(exprs)), buf_len(exprs), is_default, block};
}

Stmt *parse_stmt_switch(void) {
    Expr *expr = parse_paren_expr();
    SwitchCase *cases = NULL;
    expect_token('{');
    while (!is_token(TOKEN_EOF) && !is_token('}')) {
        buf_push(cases, parse_stmt_switch_case());
    }
    expect_token('}');
    return stmt_switch(expr, ast_dup(cases, buf_sizeof(cases)), buf_len(cases));
}

Stmt *parse_stmt(void) {
    if (is_token('{')) {
        return stmt_block(parse_stmt_block());
    } else if (match_keyword(return_keyword)) {
        Stmt *stmt = stmt_return(parse_expr());
        expect_token(';');
        return stmt;
    } else if (match_keyword(break_keyword)) {
        expect_token(';');
        return stmt_break();
    } else if (match_keyword(continue_keyword)) {
        expect_token(';');
        return stmt_continue();
    } else if (match_keyword(if_keyword)) {
        return parse_stmt_if();
    } else if (match_keyword(while_keyword)) {
        return parse_stmt_while();
    } else if (match_keyword(do_keyword)) {
        return parse_stmt_do_while();
    } else if (match_keyword(for_keyword)) {
        return parse_stmt_for();
    } else if (match_keyword(switch_keyword)) {
        return parse_stmt_switch();
    } else {
        Stmt *stmt = parse_simple_stmt();
        expect_token(';');
        return stmt;
    }
}

const char *parse_ident(void) {
    const char *name = token.name;
    expect_token(TOKEN_IDENT);
    return name;
}

Decl *parse_decl_enum(void) {
    const char *name = parse_ident();
    expect_token('{');
    EnumItem *items = NULL;
    while (!is_token(TOKEN_EOF) && !is_token('}')) {
        const char *item_name = parse_ident();
        Expr *expr = NULL;
        if (match_token('=')) {
            expr = parse_expr();
        }
        buf_push(items, (EnumItem){item_name, expr});
    }
    expect_token('}');
    return decl_enum(name, ast_dup(items, buf_sizeof(items)), buf_len(items));
}

AggregateItem parse_decl_aggregate_item(void) {
    const char **names = NULL;
    buf_push(names, parse_ident());
    while (match_token(',')) {
        buf_push(names, parse_ident());
    }
    expect_token(':');
    Typespec *type = parse_type();
    expect_token(';');
    return (AggregateItem){ast_dup(names, buf_sizeof(names)), buf_len(names), type};
}

Decl *parse_decl_aggregate(DeclKind kind) {
    assert(kind == DECL_STRUCT || kind == DECL_UNION);
    const char *name = parse_ident();
    expect_token('{');
    AggregateItem *items = NULL;
    while (!is_token(TOKEN_EOF) && !is_token('}')) {
        buf_push(items, parse_decl_aggregate_item());
    }
    expect_token('}');
    return decl_aggregate(kind, name, ast_dup(items, buf_sizeof(items)), buf_len(items));
}

Decl *parse_decl_let(void) {
    const char *name = parse_ident();
    if (match_token('=')) {
        return decl_let(name, NULL, parse_expr());
    } else if (match_token(':')) {
        Typespec *type = parse_type();
        Expr *expr = NULL;
        if (match_token('=')) {
            expr = parse_expr();
        }
        return decl_let(name, type, expr);
    } else {
        fatal_syntax_error("Expected : or = after var, got %s", temp_token_kind_str(token.kind));
        return NULL;
    }
}

Decl *parse_decl_const(void) {
    const char *name = parse_ident();
    expect_token('=');
    return decl_const(name, parse_expr());
}

Decl *parse_decl_typedef(void) {
    const char *name = parse_ident();
    expect_token('=');
    return decl_typedef(name, parse_type());
}

FnParam parse_decl_fn_param(void) {
    const char *name = parse_ident();
    expect_token(':');
    Typespec *type = parse_type();
    return (FnParam){name, type};
}

Decl *parse_decl_fn(void) {
    const char *name = parse_ident();
    expect_token('(');
    FnParam *params = NULL;
    if (!is_token(')')) {
        buf_push(params, parse_decl_fn_param());
        while (match_token(',')) {
            buf_push(params, parse_decl_fn_param());
        }
    }
    expect_token(')');
    Typespec *ret_type = NULL;
    if (match_token(':')) {
        ret_type = parse_type();
    }
    StmtBlock block = parse_stmt_block();
    return decl_fn(name, ast_dup(params, buf_sizeof(params)), buf_len(params), ret_type, block);
}

Decl *parse_decl(void) {
    if (match_keyword(enum_keyword)) {
        return parse_decl_enum();
    } else if (match_keyword(struct_keyword)) {
        return parse_decl_aggregate(DECL_STRUCT);
    } else if (match_keyword(union_keyword)) {
        return parse_decl_aggregate(DECL_UNION);
    } else if (match_keyword(let_keyword)) {
        return parse_decl_let();
    } else if (match_keyword(const_keyword)) {
        return parse_decl_const();
    } else if (match_keyword(typedef_keyword)) {
        return parse_decl_typedef();
    } else if (match_keyword(fn_keyword)) {
        return parse_decl_fn();
    } else {
        fatal_syntax_error("Expected declaration keyword, got %s", temp_token_kind_str(token.kind));
        return NULL;
    }
}

void parse_and_print_decl(const char *str) {
    init_stream(str);
    Decl *decl = parse_decl();
    print_decl(decl);
    printf("\n");
}

void parse_test(void) {
    parse_and_print_decl("fn fact(n: int): int { trace(\"fact\"); if (n == 0) { return 1; } else { return n * fact(n-1); } }");
    parse_and_print_decl("fn fact(n: int): int { p := 1; for (i := 1; i <= n; i++) { p *= i; } return p; }");
    parse_and_print_decl("let x = b == 1 ? 1+2 : 3-4");
    parse_and_print_decl("const pi = 3.14");
    parse_and_print_decl("struct Vector { x, y: float; }");
    parse_and_print_decl("union IntOrFloat { i: int; f: float; }");
    parse_and_print_decl("typedef Vectors = Vector[1+2]");
}
