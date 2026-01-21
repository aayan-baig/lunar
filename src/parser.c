#include "parser.h"
#include <string.h>
#include <stdio.h>

static void next(Parser *p) {
    p->cur = lexer_next(p->lx);
}

static int is(Parser *p, TokenKind k) {
    return p->cur.kind == k;
}

static int accept(Parser *p, TokenKind k) {
    if (is(p, k)) {
        next(p);
        return 1;
    }
    return 0;
}

static void error_at(Parser *p, Span sp, const char *msg) {
    p->had_error = 1;
    diag_error(sp, "%s", msg);
}

static int expect(Parser *p, TokenKind k, const char *what) {
    if (is(p, k)) {
        next(p);
        return 1;
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "expected %s", what);
    error_at(p, p->cur.span, buf);
    return 0;
}

static StrView tok_strview(Token t) {
    StrView sv;
    sv.ptr = t.start;
    sv.len = t.length;
    return sv;
}

// ----- Forward decls -----
static FnDecl *parse_fn(Parser *p);
static void parse_block(Parser *p, Stmt ***out_stmts, size_t *out_len);

static Stmt *parse_stmt(Parser *p);

static Expr *parse_expr(Parser *p);
static Expr *parse_assignment(Parser *p);
static Expr *parse_equality(Parser *p);
static Expr *parse_compare(Parser *p);
static Expr *parse_term(Parser *p);
static Expr *parse_factor(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_call(Parser *p);
static Expr *parse_primary(Parser *p);

void parser_init(Parser *p, Lexer *lx, Arena *arena) {
    p->lx = lx;
    p->arena = arena;
    p->had_error = 0;
    next(p);
}

Program *parse_program(Parser *p) {
    Program *prog = ast_new_program(p->arena);
    if (!prog) return NULL;

    FnDecl **fns = NULL;
    size_t fns_len = 0;
    size_t fns_cap = 0;

    while (!is(p, TOK_EOF)) {
        if (!is(p, TOK_KW_FUNCT)) {
            error_at(p, p->cur.span, "top-level: expected 'funct'");
            // best-effort recovery: skip one token
            next(p);
            continue;
        }

        FnDecl *fn = parse_fn(p);
        if (!fn) break;

        if (fns_len == fns_cap) {
            size_t new_cap = fns_cap ? fns_cap * 2 : 8;
            FnDecl **nf = (FnDecl **)arena_alloc(p->arena, new_cap * sizeof(FnDecl *), _Alignof(FnDecl *));
            if (!nf) return NULL;
            if (fns) memcpy(nf, fns, fns_len * sizeof(FnDecl *));
            fns = nf;
            fns_cap = new_cap;
        }
        fns[fns_len++] = fn;
    }

    prog->fns = fns;
    prog->fns_len = fns_len;
    return prog;
}

// funct <ident> ( <params>? ) ret <type> { <stmts>* }
static FnDecl *parse_fn(Parser *p) {
    Token funct_tok = p->cur;
    expect(p, TOK_KW_FUNCT, "'funct'");

    Token name = p->cur;
    if (!expect(p, TOK_IDENT, "function name")) return NULL;

    FnDecl *fn = ast_new_fn(p->arena);
    if (!fn) return NULL;

    fn->name = tok_strview(name);
    fn->span = funct_tok.span;

    expect(p, TOK_LPAREN, "'('");

    // params: ident (':' ident)? (',' ...)?
    Param *params = NULL;
    size_t params_len = 0;
    size_t params_cap = 0;

    if (!is(p, TOK_RPAREN)) {
        for (;;) {
            Token p_name = p->cur;
            if (!expect(p, TOK_IDENT, "parameter name")) break;

            StrView type_name = (StrView){0};
            if (accept(p, TOK_COLON)) {
                Token ty = p->cur;
                expect(p, TOK_IDENT, "type name");
                type_name = tok_strview(ty);
            }

            if (params_len == params_cap) {
                size_t new_cap = params_cap ? params_cap * 2 : 4;
                Param *np = (Param *)arena_alloc(p->arena, new_cap * sizeof(Param), _Alignof(Param));
                if (!np) return NULL;
                if (params) memcpy(np, params, params_len * sizeof(Param));
                params = np;
                params_cap = new_cap;
            }

            Param pr;
            pr.name = tok_strview(p_name);
            pr.type_name = type_name;
            pr.span = p_name.span;
            params[params_len++] = pr;

            if (accept(p, TOK_COMMA)) continue;
            break;
        }
    }

    expect(p, TOK_RPAREN, "')'");

    // required return type: ret Ident
    fn->return_type = (StrView){0};
    expect(p, TOK_KW_RET, "'ret'");
    Token rt = p->cur;
    expect(p, TOK_IDENT, "return type");
    fn->return_type = tok_strview(rt);

    // body
    expect(p, TOK_LBRACE, "'{'");

    Stmt **body = NULL;
    size_t body_len = 0;
    parse_block(p, &body, &body_len);

    fn->params = params;
    fn->params_len = params_len;
    fn->body = body;
    fn->body_len = body_len;

    return fn;
}

static void parse_block(Parser *p, Stmt ***out_stmts, size_t *out_len) {
    Stmt **stmts = NULL;
    size_t len = 0;
    size_t cap = 0;

    while (!is(p, TOK_EOF) && !is(p, TOK_RBRACE)) {
        Stmt *s = parse_stmt(p);
        if (!s) {
            // best-effort recovery: skip one token
            next(p);
            continue;
        }

        if (len == cap) {
            size_t new_cap = cap ? cap * 2 : 8;
            Stmt **ns = (Stmt **)arena_alloc(p->arena, new_cap * sizeof(Stmt *), _Alignof(Stmt *));
            if (!ns) return;
            if (stmts) memcpy(ns, stmts, len * sizeof(Stmt *));
            stmts = ns;
            cap = new_cap;
        }
        stmts[len++] = s;
    }

    expect(p, TOK_RBRACE, "'}'");

    *out_stmts = stmts;
    *out_len = len;
}

// stmt:
//   let (mut)? ident ( : ident )? = expr ;
//   return expr? ;
//   expr ;
static Stmt *parse_stmt(Parser *p) {
    if (accept(p, TOK_KW_LET)) {
        Span sp = p->cur.span; // best effort (already advanced past 'let')
        int is_mut = accept(p, TOK_KW_MUT);

        Token name = p->cur;
        if (!expect(p, TOK_IDENT, "variable name")) return NULL;

        StrView type_name = (StrView){0};
        if (accept(p, TOK_COLON)) {
            Token ty = p->cur;
            expect(p, TOK_IDENT, "type name");
            type_name = tok_strview(ty);
        }

        expect(p, TOK_EQ, "'='");

        Expr *init = parse_expr(p);
        expect(p, TOK_SEMI, "';'");

        Stmt *s = ast_new_stmt(p->arena, STMT_LET, name.span);
        if (!s) return NULL;
        s->as.let_stmt.is_mut = is_mut;
        s->as.let_stmt.name = tok_strview(name);
        s->as.let_stmt.type_name = type_name;
        s->as.let_stmt.init = init;
        s->span = name.span;
        (void)sp;
        return s;
    }

    if (accept(p, TOK_KW_RETURN)) {
        Span sp = p->cur.span; // best effort (already advanced past 'return')
        Expr *v = NULL;
        if (!is(p, TOK_SEMI)) {
            v = parse_expr(p);
        }
        expect(p, TOK_SEMI, "';'");
        Stmt *s = ast_new_stmt(p->arena, STMT_RETURN, sp);
        if (!s) return NULL;
        s->as.ret_stmt.value = v;
        return s;
    }

    // expression statement
    Expr *e = parse_expr(p);
    expect(p, TOK_SEMI, "';'");
    Stmt *s = ast_new_stmt(p->arena, STMT_EXPR, e ? e->span : p->cur.span);
    if (!s) return NULL;
    s->as.expr_stmt.expr = e;
    return s;
}

// --- Expression parsing (precedence) ---
// expr -> assignment
static Expr *parse_expr(Parser *p) {
    return parse_assignment(p);
}

// assignment -> equality ( '=' assignment )?
static Expr *parse_assignment(Parser *p) {
    Expr *lhs = parse_equality(p);
    if (!lhs) return NULL;

    if (accept(p, TOK_EQ)) {
        // only allow "name = expr" for now
        if (lhs->kind != EXPR_NAME) {
            error_at(p, lhs->span, "left side of assignment must be a name");
        }
        Expr *rhs = parse_assignment(p);

        Expr *e = ast_new_expr(p->arena, EXPR_ASSIGN, lhs->span);
        if (!e) return NULL;
        e->as.assign.name = lhs->as.str;
        e->as.assign.value = rhs;
        return e;
    }

    return lhs;
}

// equality -> compare ( (== | !=) compare )*
static Expr *parse_equality(Parser *p) {
    Expr *e = parse_compare(p);
    while (is(p, TOK_EQEQ) || is(p, TOK_UNEQ)) {
        Token op = p->cur;
        next(p);
        Expr *rhs = parse_compare(p);

        BinaryOp bop = (op.kind == TOK_EQEQ) ? BOP_EQ : BOP_NE;
        Expr *n = ast_new_expr(p->arena, EXPR_BINARY, op.span);
        if (!n) return NULL;
        n->as.binary.op = bop;
        n->as.binary.lhs = e;
        n->as.binary.rhs = rhs;
        e = n;
    }
    return e;
}

// compare -> term ( (< | <= | > | >=) term )*
static Expr *parse_compare(Parser *p) {
    Expr *e = parse_term(p);
    while (is(p, TOK_LT) || is(p, TOK_LTEQ) || is(p, TOK_GT) || is(p, TOK_GTEQ)) {
        Token op = p->cur;
        next(p);
        Expr *rhs = parse_term(p);

        BinaryOp bop = BOP_LT;
        if (op.kind == TOK_LT) bop = BOP_LT;
        else if (op.kind == TOK_LTEQ) bop = BOP_LTE;
        else if (op.kind == TOK_GT) bop = BOP_GT;
        else if (op.kind == TOK_GTEQ) bop = BOP_GTE;

        Expr *n = ast_new_expr(p->arena, EXPR_BINARY, op.span);
        if (!n) return NULL;
        n->as.binary.op = bop;
        n->as.binary.lhs = e;
        n->as.binary.rhs = rhs;
        e = n;
    }
    return e;
}

// term -> factor ( (+ | -) factor )*
static Expr *parse_term(Parser *p) {
    Expr *e = parse_factor(p);
    while (is(p, TOK_PLUS) || is(p, TOK_MINUS)) {
        Token op = p->cur;
        next(p);
        Expr *rhs = parse_factor(p);

        BinaryOp bop = (op.kind == TOK_PLUS) ? BOP_ADD : BOP_SUB;

        Expr *n = ast_new_expr(p->arena, EXPR_BINARY, op.span);
        if (!n) return NULL;
        n->as.binary.op = bop;
        n->as.binary.lhs = e;
        n->as.binary.rhs = rhs;
        e = n;
    }
    return e;
}

// factor -> unary ( (* | /) unary )*
static Expr *parse_factor(Parser *p) {
    Expr *e = parse_unary(p);
    while (is(p, TOK_STAR) || is(p, TOK_SLASH)) {
        Token op = p->cur;
        next(p);
        Expr *rhs = parse_unary(p);

        BinaryOp bop = (op.kind == TOK_STAR) ? BOP_MUL : BOP_DIV;

        Expr *n = ast_new_expr(p->arena, EXPR_BINARY, op.span);
        if (!n) return NULL;
        n->as.binary.op = bop;
        n->as.binary.lhs = e;
        n->as.binary.rhs = rhs;
        e = n;
    }
    return e;
}

// unary -> ('-' | '!') unary | call
static Expr *parse_unary(Parser *p) {
    if (is(p, TOK_MINUS) || is(p, TOK_EXCL)) {
        Token op = p->cur;
        next(p);
        Expr *rhs = parse_unary(p);

        Expr *e = ast_new_expr(p->arena, EXPR_UNARY, op.span);
        if (!e) return NULL;
        e->as.unary.op = (op.kind == TOK_MINUS) ? UOP_NEG : UOP_NOT;
        e->as.unary.rhs = rhs;
        return e;
    }
    return parse_call(p);
}

// call -> primary ( '(' args? ')' )*
static Expr *parse_call(Parser *p) {
    Expr *e = parse_primary(p);
    for (;;) {
        if (!accept(p, TOK_LPAREN)) break;

        Expr **args = NULL;
        size_t args_len = 0;
        size_t args_cap = 0;

        if (!is(p, TOK_RPAREN)) {
            for (;;) {
                Expr *a = parse_expr(p);
                if (!a) break;

                if (args_len == args_cap) {
                    size_t new_cap = args_cap ? args_cap * 2 : 4;
                    Expr **na = (Expr **)arena_alloc(p->arena, new_cap * sizeof(Expr *), _Alignof(Expr *));
                    if (!na) return NULL;
                    if (args) memcpy(na, args, args_len * sizeof(Expr *));
                    args = na;
                    args_cap = new_cap;
                }
                args[args_len++] = a;

                if (accept(p, TOK_COMMA)) continue;
                break;
            }
        }

        expect(p, TOK_RPAREN, "')'");

        Expr *call = ast_new_expr(p->arena, EXPR_CALL, e->span);
        if (!call) return NULL;
        call->as.call.callee = e;
        call->as.call.args = args;
        call->as.call.args_len = args_len;
        e = call;
    }
    return e;
}

// primary -> INT | STRING | true | false | IDENT | '(' expr ')'
static Expr *parse_primary(Parser *p) {
    Token t = p->cur;

    if (accept(p, TOK_INT)) {
        Expr *e = ast_new_expr(p->arena, EXPR_INT, t.span);
        if (!e) return NULL;
        e->as.int_val = t.int_val;
        return e;
    }

    if (accept(p, TOK_STRING)) {
        Expr *e = ast_new_expr(p->arena, EXPR_STRING, t.span);
        if (!e) return NULL;
        e->as.str = tok_strview(t);
        return e;
    }

    if (accept(p, TOK_KW_TRUE)) {
        Expr *e = ast_new_expr(p->arena, EXPR_BOOL, t.span);
        if (!e) return NULL;
        e->as.bool_val = 1;
        return e;
    }

    if (accept(p, TOK_KW_FALSE)) {
        Expr *e = ast_new_expr(p->arena, EXPR_BOOL, t.span);
        if (!e) return NULL;
        e->as.bool_val = 0;
        return e;
    }

    if (accept(p, TOK_IDENT)) {
        Expr *e = ast_new_expr(p->arena, EXPR_NAME, t.span);
        if (!e) return NULL;
        e->as.str = tok_strview(t);
        return e;
    }

    if (accept(p, TOK_LPAREN)) {
        Expr *e = parse_expr(p);
        expect(p, TOK_RPAREN, "')'");
        return e;
    }

    error_at(p, p->cur.span, "expected expression");
    return NULL;
}
