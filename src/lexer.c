#include "lexer.h"
#include <ctype.h>
#include <string.h>

static char peek(){}

static char peek2(){}

static char advance(){}

static int match(){}

static Span span_here(){}

static void skip_whitespaces_and_comments(){}

static Token make_token(){}

static Token lex_number(){}

static int is_ident_start(){}

static int is_ident_cont(){}

static TokenKind keyword_or_indent(){}

static Token lex_ident_or_kw(){}

static Token lex_string(){}

void lexer_init(){}

const char *token_kind_name(){}