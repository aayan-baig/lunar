#ifndef LUNAR_AST_H
#define LUNAR_AST_H

#include <stddef.h>
#include <stdint.h>
#include "diag.h"

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum {
    EXPR_INT = 1,
    EXPR_STRING,
    EXPR_NAME,
    EXPR_BOOL,

    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_ASSIGN,
    EXPR_CALL,
} ExprKind;

typedef enum {
    STMT_LET = 1,
    STMT_RETURN,
    STMT_EXPR,
} StmtKind;

typedef enum {
    UOP_NEG = 1,   // -
    UOP_NOT,       // !
} UnaryOp;

typedef enum {
    BOP_ADD = 1, BOP_SUB, BOP_MUL, BOP_DIV,
    BOP_EQ, BOP_NE,
    BOP_LT, BOP_LTE,
    BOP_GT, BOP_GTE,
} BinaryOp;

typedef struct {
    const char *ptr;
    size_t len;
} StrView;

typedef struct {
    StrView name;
    StrView type_name; // (len==0 means omitted)
    Span span;
} Param;

typedef struct {
    StrView name;
    StrView return_type; // optional
    Param *params;
    size_t params_len;

    Stmt **body;
    size_t body_len;

    Span span;
} FnDecl;

typedef struct {
    FnDecl **fns;
    size_t fns_len;
} Program;

// --- Expr / Stmt nodes ---

struct Expr {
    ExprKind kind;
    Span span;
    union {
        int64_t int_val;
        int bool_val; // 0/1
        StrView str;  // for string literal or name

        struct {
            UnaryOp op;
            Expr *rhs;
        } unary;

        struct {
            BinaryOp op;
            Expr *lhs;
            Expr *rhs;
        } binary;

        struct {
            StrView name;
            Expr *value;
        } assign;

        struct {
            Expr *callee;      // usually EXPR_NAME for now
            Expr **args;
            size_t args_len;
        } call;
    } as;
};

struct Stmt {
    StmtKind kind;
    Span span;
    union {
        struct {
            int is_mut;
            StrView name;
            StrView type_name; // optional
            Expr *init;         // required for v0
        } let_stmt;

        struct {
            Expr *value; // optional; NULL means "return;"
        } ret_stmt;

        struct {
            Expr *expr;
        } expr_stmt;
    } as;
};

// --- Arena allocator (simple bump arena) ---

typedef struct {
    unsigned char *buf;
    size_t cap;
    size_t used;
} Arena;

void arena_init(Arena *a, size_t initial_cap);
void arena_free(Arena *a);
void *arena_alloc(Arena *a, size_t size, size_t align);

// Helpers for AST allocations
Expr *ast_new_expr(Arena *a, ExprKind k, Span sp);
Stmt *ast_new_stmt(Arena *a, StmtKind k, Span sp);

FnDecl *ast_new_fn(Arena *a);
Program *ast_new_program(Arena *a);

#endif
