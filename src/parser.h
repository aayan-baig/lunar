#ifndef LUNAR_PARSER_H
#define LUNAR_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lx;
    Arena *arena;

    Token cur;
    int had_error;
} Parser;

void parser_init(Parser *p, Lexer *lx, Arena *arena);

// parses whole file into Program*. returns NULL on hard failure.
Program *parse_program(Parser *p);

#endif
