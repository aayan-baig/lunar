#include "ast.h"
#include <stdlib.h>
#include <string.h>

static size_t align_up(size_t x, size_t a) {
    return (x + (a - 1)) & ~(a - 1);
}

void arena_init(Arena *a, size_t initial_cap) {
    a->buf = (unsigned char *)malloc(initial_cap);
    a->cap = a->buf ? initial_cap : 0;
    a->used = 0;
}

void arena_free(Arena *a) {
    free(a->buf);
    a->buf = NULL;
    a->cap = 0;
    a->used = 0;
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
    if (align == 0) align = 1;
    size_t start = align_up(a->used, align);
    size_t end = start + size;

    if (end > a->cap) {
        size_t new_cap = a->cap ? a->cap : 1024;
        while (new_cap < end) new_cap *= 2;

        unsigned char *nb = (unsigned char *)realloc(a->buf, new_cap);
        if (!nb) return NULL;
        a->buf = nb;
        a->cap = new_cap;
    }

    void *p = a->buf + start;
    a->used = end;
    memset(p, 0, size);
    return p;
}

Expr *ast_new_expr(Arena *a, ExprKind k, Span sp) {
    Expr *e = (Expr *)arena_alloc(a, sizeof(Expr), _Alignof(Expr));
    if (!e) return NULL;
    e->kind = k;
    e->span = sp;
    return e;
}

Stmt *ast_new_stmt(Arena *a, StmtKind k, Span sp) {
    Stmt *s = (Stmt *)arena_alloc(a, sizeof(Stmt), _Alignof(Stmt));
    if (!s) return NULL;
    s->kind = k;
    s->span = sp;
    return s;
}

FnDecl *ast_new_fn(Arena *a) {
    FnDecl *f = (FnDecl *)arena_alloc(a, sizeof(FnDecl), _Alignof(FnDecl));
    return f;
}

Program *ast_new_program(Arena *a) {
    Program *p = (Program *)arena_alloc(a, sizeof(Program), _Alignof(Program));
    return p;
}
