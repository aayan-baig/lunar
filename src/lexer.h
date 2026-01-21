#ifndef LUNAR_LEXER_H
#define LUNAR_LEXER_H

#include <stddef.h>
#include <stdint.h>
#include "diag.h" // for error logger;

typedef enum {
    TOK_EOF = 0,

    // idents + literals
    TOK_IDENT,
    TOK_INT,
    TOK_STRING,

    // keywords
    TOK_KW_FUNCT,   // funct
    TOK_KW_RET,     // ret 
    TOK_KW_LET,
    TOK_KW_MUT,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_WHILE,
    TOK_KW_RETURN,  // return (statement)
    TOK_KW_TRUE,
    TOK_KW_FALSE,

    // operators & punctuation
    TOK_LPAREN,
    TOK_RPAREN,

    TOK_LBRACE,
    TOK_RBRACE,

    TOK_LBRACK,
    TOK_RBRACK,

    TOK_COMMA,
    TOK_SEMI,
    TOK_COLON,
    TOK_DOT,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,

    TOK_EQ,
    TOK_EQEQ,   // ==
    TOK_UNEQ,   // !=
    TOK_EXCL,   // !
    TOK_LT,     // <
    TOK_LTEQ,   // <=
    TOK_GT,     // >
    TOK_GTEQ,   // >=

} TokenKind;



typedef struct {
    TokenKind kind;
    Span span;
    const char *start;
    size_t length;
    int64_t int_val;
} Token;

typedef struct{
    const char *path;
    const char *src;
    size_t len;

    size_t i; // byte index into src
    size_t line;
    size_t col;
    int had_error;
}Lexer;

void lexer_init(Lexer *lx, const char *path, const char *src, size_t len);
Token lexer_next(Lexer *lx);

const char *token_kind_name(TokenKind k);

#endif