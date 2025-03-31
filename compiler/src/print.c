#include "ast.h"
#include "lex.h"

int indent;

void print_newline(void) {
    printf("\n%.*s", 2 * indent, "                                                                      ");
}

void print_typespec(Typespec *type) {
    Typespec *t = type;
    switch (t->kind) {
    case TYPESPEC_IDENT:
        printf("%s", t->name);
        break;
    case TYPESPEC_FN:
        printf("(fn (");
        for (Typespec **it = t->fn.args; it != t->fn.args + t->fn.num_args; it++) {
            printf(" ");
            print_typespec(*it);
        }
        printf(") ");
        print_typespec(t->fn.ret);
        printf(")");
        break;
    case TYPESPEC_ARRAY:
        printf("(array ");
        print_typespec(t->array.elem);
        printf(" ");
        print_expr(t->array.size);
        printf(")");
        break;
    case TYPESPEC_PTR:
        printf("(ptr ");
        print_typespec(t->ptr.elem);
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}

void print_expr(Expr *expr) {
    Expr *e = expr;
    switch (e->kind) {
    case EXPR_INT:
        printf("%llu", e->int_val);
        break;
    case EXPR_FLOAT:
        printf("%f", e->float_val);
        break;
    case EXPR_STR:
        printf("\"%s\"", e->str_val);
        break;
    case EXPR_IDENT:
        printf("%s", e->name);
        break;
    case EXPR_CAST:
        printf("(cast ");
        print_typespec(e->cast.type);
        printf(" ");
        print_expr(e->cast.expr);
        printf(")");
        break;
    case EXPR_CALL:
        printf("(");
        print_expr(e->call.expr);
        for (Expr **it = e->call.args; it != e->call.args + e->call.num_args; it++) {
            printf(" ");
            print_expr(*it);
        }
        printf(")");
        break;
    case EXPR_INDEX:
        printf("(index ");
        print_expr(e->index.expr);
        printf(" ");
        print_expr(e->index.index);
        printf(")");
        break;
    case EXPR_FIELD:
        printf("(field ");
        print_expr(e->field.expr);
        printf(" %s)", e->field.name);
        break;
    case EXPR_COMPOUND:
        printf("(compound ");
        if (e->compound.type) {
            print_typespec(e->compound.type);
        } else {
            printf("nil");
        }
        for (Expr **it = e->compound.args; it != e->compound.args + e->compound.num_args; it++) {
            printf(" ");
            print_expr(*it);
        }
        printf(")");
        break;
    case EXPR_UNARY:
        printf("(%s ", temp_token_kind_str(e->unary.op));
        print_expr(e->unary.expr);
        printf(")");
        break;
    case EXPR_BINARY:
        printf("(%s ", temp_token_kind_str(e->binary.op));
        print_expr(e->binary.left);
        printf(" ");
        print_expr(e->binary.right);
        printf(")");
        break;
    case EXPR_TERNARY:
        printf("(? ");
        print_expr(e->ternary.cond);
        printf(" ");
        print_expr(e->ternary.then_expr);
        printf(" ");
        print_expr(e->ternary.else_expr);
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}

void print_stmt_block(StmtBlock block) {
    printf("(block");
    indent++;
    for (Stmt **it = block.stmts; it != block.stmts + block.num_stmts; it++) {
        print_newline();
        print_stmt(*it);
    }
    indent--;
    printf(")");
}

void print_stmt(Stmt *stmt) {
    Stmt *s = stmt;
    switch (s->kind) {
    case STMT_RETURN:
        printf("(return ");
        print_expr(s->return_stmt.expr);
        printf(")");
        break;
    case STMT_BREAK:
        printf("(break)");
        break;
    case STMT_CONTINUE:
        printf("(continue)");
        break;
    case STMT_BLOCK:
        print_stmt_block(s->block);
        break;
    case STMT_IF:
        printf("(if ");
        print_expr(s->if_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(s->if_stmt.then_block);
        for (ElseIf *it = s->if_stmt.elseifs; it != s->if_stmt.elseifs + s->if_stmt.num_elseifs; it++) {
            print_newline();
            printf("elseif ");
            print_expr(it->cond);
            print_newline();
            print_stmt_block(it->block);
        }
        if (s->if_stmt.else_block.num_stmts != 0) {
            print_newline();
            printf("else ");
            print_newline();
            print_stmt_block(s->if_stmt.else_block);
        }
        indent--;
        printf(")");
        break;
    case STMT_WHILE:
        printf("(while ");
        print_expr(s->while_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(s->while_stmt.block);
        indent--;
        printf(")");
        break;
    case STMT_DO_WHILE:
        printf("(do-while ");
        print_expr(s->while_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(s->while_stmt.block);
        indent--;
        printf(")");
        break;
    case STMT_FOR:
        printf("(for ");
        print_stmt(s->for_stmt.init);
        print_expr(s->for_stmt.cond);
        print_stmt(s->for_stmt.next);
        indent++;
        print_newline();
        print_stmt_block(s->for_stmt.block);
        indent--;
        printf(")");
        break;
    case STMT_SWITCH:
        printf("(switch ");
        print_expr(s->switch_stmt.expr);
        indent++;
        for (SwitchCase *it = s->switch_stmt.cases; it != s->switch_stmt.cases + s->switch_stmt.num_cases; it++) {
            print_newline();
            printf("(case (%s", it->is_default ? " default" : "");
            for (Expr **expr = it->exprs; expr != it->exprs + it->num_exprs; expr++) {
                printf(" ");
                print_expr(*expr);
            }
            printf(" ) ");
            indent++;
            print_newline();
            print_stmt_block(it->block);
            indent--;
        }
        indent--;
        printf(")");
        break;
    case STMT_ASSIGN:
        printf("(%s ", token_kind_name(s->assign.op));
        print_expr(s->assign.left);
        if (s->assign.right) {
            printf(" ");
            print_expr(s->assign.right);
        }
        printf(")");
        break;
    case STMT_INIT:
        printf("(:= %s ", s->init.name);
        print_expr(s->init.expr);
        printf(")");
        break;
    case STMT_EXPR:
        print_expr(s->expr);
        break;
    default:
        assert(0);
        break;
    }
}

void print_aggregate_decl(Decl *decl) {
    Decl *d = decl;
    for (AggregateItem *it = d->aggregate.items; it != d->aggregate.items + d->aggregate.num_items; it++) {
        print_newline();
        printf("(");
        print_typespec(it->types);
        for (const char **name = it->names; name != it->names + it->num_names; name++) {
            printf(" %s", *name);
        }
        printf(")");
    }
}

void print_decl(Decl *decl) {
    Decl *d = decl;
    switch (d->kind) {
    case DECL_ENUM:
        printf("(enum %s", d->name);
        indent++;
        for (EnumItem *it = d->enum_decl.items; it != d->enum_decl.items + d->enum_decl.num_items; it++) {
            print_newline();
            printf("(%s ", it->name);
            if (it->expr) {
                print_expr(it->expr);
            } else {
                printf("nil");
            }
            printf(")");
        }
        indent--;
        printf(")");
        break;
    case DECL_STRUCT:
        printf("(struct %s", d->name);
        indent++;
        print_aggregate_decl(d);
        indent--;
        printf(")");
        break;
    case DECL_UNION:
        printf("(union %s", d->name);
        indent++;
        print_aggregate_decl(d);
        indent--;
        printf(")");
        break;
    case DECL_LET:
        printf("(let %s ", d->name);
        if (d->let.type) {
            print_typespec(d->let.type);
        } else {
            printf("nil");
        }
        printf(" ");
        print_expr(d->let.expr);
        printf(")");
        break;
    case DECL_CONST:
        printf("(const %s ", d->name);
        print_expr(d->const_decl.expr);
        printf(")");
        break;
    case DECL_TYPEDEF:
        printf("(typedef %s ", d->name);
        print_typespec(d->typedef_decl.type);
        printf(")");
        break;
    case DECL_FN:
        printf("(fn %s ", d->name);
        printf("(");
        for (FnParam *it = d->fn.params; it != d->fn.params + d->fn.num_params; it++) {
            printf(" %s ", it->name);
            print_typespec(it->type);
        }
        printf(" ) ");
        if (d->fn.ret_type) {
            print_typespec(d->fn.ret_type);
        } else {
            printf("nil");
        }
        indent++;
        print_newline();
        print_stmt_block(d->fn.block);
        indent--;
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}

void print_test(void) {
    Expr *exprs[] = {
        expr_binary('+', expr_int(1), expr_int(2)),
        expr_unary('-', expr_float(3.14)),
        expr_ternary(expr_ident("flag"), expr_str("true"), expr_str("false")),
        expr_field(expr_ident("person"), "name"),
        expr_call(expr_ident("fact"), (Expr *[]){expr_int(42)}, 1),
        expr_index(expr_field(expr_ident("person"), "siblings"), expr_int(3)),
        expr_cast(typespec_ptr(typespec_ident("int")), expr_ident("void_ptr")),
        expr_compound(typespec_ident("Vector"), (Expr *[]){expr_int(1), expr_int(2)}, 2),
    };
    for (Expr **it = exprs; it != exprs + sizeof(exprs) / sizeof(*exprs); it++) {
        print_expr(*it);
        printf("\n");
    }

    // Statements
    Stmt *stmts[] = {
        stmt_return(expr_int(42)),
        stmt_break(),
        stmt_continue(),
        stmt_block(
            (StmtBlock){
                (Stmt *[]){
                    stmt_break(),
                    stmt_continue()},
                2,
            }),
        stmt_expr(expr_call(expr_ident("print"), (Expr *[]){expr_int(1), expr_int(2)}, 2)),
        stmt_init("x", expr_int(42)),
        stmt_if(
            expr_ident("flag1"),
            (StmtBlock){
                (Stmt *[]){
                    stmt_return(expr_int(1))},
                1,
            },
            (ElseIf[]){
                expr_ident("flag2"),

                (StmtBlock){
                    (Stmt *[]){
                        stmt_return(expr_int(2))},
                    1,
                }},
            1,
            (StmtBlock){
                (Stmt *[]){
                    stmt_return(expr_int(3))},
                1,
            }),
        stmt_while(
            expr_ident("running"),
            (StmtBlock){
                (Stmt *[]){
                    stmt_assign(TOKEN_ADD_ASSIGN, expr_ident("i"), expr_int(16)),
                },
                1,
            }),
        stmt_switch(
            expr_ident("val"),
            (SwitchCase[]){
                {
                    (Expr *[]){expr_int(3), expr_int(4)},
                    2,
                    false,
                    (StmtBlock){
                        (Stmt *[]){stmt_return(expr_ident("val"))},
                        1,
                    },
                },
                {
                    (Expr *[]){expr_int(1)},
                    1,
                    true,
                    (StmtBlock){
                        (Stmt *[]){stmt_return(expr_int(0))},
                        1,
                    },
                },
            },
            2),
    };
    for (Stmt **it = stmts; it != stmts + sizeof(stmts) / sizeof(*stmts); it++) {
        print_stmt(*it);
        printf("\n");
    }
}
