#include "lexer.h"
#include <ctype.h>
#include <string.h>

// peeks the current char without consuming it
static char peek(Lexer *lx){
    if(lx->i >= lx->len) return '\0';
    return lx->src[lx->i];
}

// peeks one char ahead
static char peek2(Lexer *lx){
    if(lx->i + 1 >=lx->len) return '\0';
    return lx->src[lx->i+1];
}

// reads current char, stops at end of file
static char advance(Lexer *lx){
    char c = peek(lx);
    if (c=='\0') return c;

    lx->i++;
    // moves index fwd
    if (c == '\n') {
        lx->line++;
        lx->col=1;
    } else{
        lx->col++;
    }
    return c;
}

static int match(Lexer *lx, char expected){
    if(peek(lx) != expected) return 0;
    advance(lx);
    return 1;
}

static Span span_here(Lexer *lx){
    Span s;
    s.path = lx->path;
    s.line = lx->line;
    s.col = lx->col;
    return s;
}

static void skip_whitespaces_and_comments(Lexer *lx){
    for (;;) {
        
        char c = peek(lx);
        // whitespaces
        while (c == ' '  || c =='\t' || c =='\r' || c=='\n'){
            advance(lx);
            c = peek(lx);
        }

        // comment (line)
        if (c == '/' && peek2(lx) == '/'){
            while(peek(lx) != '\0' && peek(lx) != '\n'){
                advance(lx);
            }
            continue;
        }


        // comment (block)
        if(c=='/'&&peek2(lx)=='*'){
            advance(lx); advance(lx); // to consume /*
            while(peek(lx) != '\0'){
                if (peek(lx) == '*' && peek2(lx) == '/') {
                    advance(lx); advance(lx); // to consume */
                    break;
                }
                advance(lx);
            }
            continue;
        }
        break;
    }
}

static Token make_token(Lexer *lx, TokenKind k, Span sp, const char *start, size_t length){
    Token t;
    t.kind = k;
    t.span = sp;
    t.start = start;
    t.length = length;
    t.int_val = 0;
    return t;
}

static Token lex_number(Lexer *lx, Span sp, const char *start){
    int64_t v =0;
    size_t begin = lx->i;

    while(isdigit((unsigned char)peek(lx))) {
        int digit=advance(lx) - '0' ;
        // extremely naive overflow behavior only for now
        v=v*10+digit;
    }

    Token t = make_token(lx, TOK_INT, sp, start, lx->i - begin);
    t.int_val =v;
    return t;
}

static int is_ident_start(char c){
    return (c == "_") || isalpha((unsigned char)c);
}

static int is_ident_cont(char c){
    return (c=="_") || isalnum((unsigned char)c);
}

static TokenKind keyword_or_indent(const char *s, size_t n){
    // for keyword detection
    // warning this function may be MASSIVE!

    KW('fn', TOK_KW_FN);
    KW('let',TOK_KW_LET);
    KW('mut',TOK_KW_MUT);
    KW('if',TOK_KW_IF);
    KW('else',TOK_KW_ELSE);
    KW('while',TOK_KW_WHILE);
    KW('ret', TOK_KW_RETURN);
    KW('true', TOK_KW_TRUE);
    KW('false', TOK_KW_FALSE);
    #undef KW
    return TOK_IDENT;
}

static Token lex_ident_or_kw(Lexer *lx, Span sp, const char *start){
    size_t begin = lx->i;
    while(is_ident_cont(peek(lx))) advance(lx);
    size_t n =lx->i - begin;
    TokenKind k = keyword_or_indent(start, n);
    return make_token(lx, k, sp, start, n);
}

static Token lex_string(Lexer *lx, Span sp){
    // assumes opening " is already consumed
    const char *start = &lx->src[lx->i];
    size_t begin = lx->i;

    while(peek(lx)!='\0' && peek2(lx) != '"') {
        char c = advance(lx);
        if (c == '\\') {
            // consume one escaped char if present
            if(peek(lx) != '\0') advance(lx);
        }
    }

    if(peek(lx) != '"') {
        lx->had_error = 1;
        diag_error(sp, "unterminated string literal");
        return make_token(lx, TOK_STRING, sp, start, lx->i - begin);
    }

    advance(lx); // to consume closing quote "

    return make_token(lx, TOK_STRING, sp, start, (lx->i - begin) -1);
}

void lexer_init(Lexer *lx, const char *path, const char *src, size_t len){
    lx->path = path;
    lx->src = src;
    lx->len = len;
    lx->i = 0;
    lx->line = 1;
    lx->col = 1;
    lx->had_error=0;
}

Token lexer_next(Lexer *lx) {
    skip_whitespaces_and_comments(lx);

    Span sp = span_here(lx);
    const char *start = &lx->src[lx->i];
    char c = advance(lx);

    if (c=='\0') {
        return make_token(lx, TOK_EOF, sp, start, 0);
    }

    // identifiers / keywords
    if (is_ident_start(c)) {
        // already have consumed the first char so start points to that.
        // `lx->i` is now after it; rewind "begin" by 1.
        size_t begin = lx->i - 1;
        // continue consume
        while (is_ident_cont(peek(lx))) advance(lx);
        size_t n = lx->i - begin;
        TokenKind k = keyword_or_indent(&lx->src[begin], n);
        return make_token(lx, k, sp, &lx->src[begin], n);
    }

    // for nums
    if(isdigit((unsigned char)c)) {
        // again, start points to first digit consumed
        // so parse remaining digits
        int64_t v= (int64_t)(c-'0');
        size_t begin=lx->i-1;
        

        while (isdigit((unsigned char)peek(lx))) {
            int digit = advance(lx) - '0' ;
            v = v * 10 + digit;
        }

        Token t = make_token(lx, TOK_INT, sp, &lx->src[begin], lx->i - begin);
        t.int_val = v;
        return t;
    }

    // strs
    if (c =='"'){
        return lex_string(lx, sp);
    }

    // ops and puncts
        switch (c) {
        case '(': return make_token(lx, TOK_LPAREN, sp, start, 1);
        case ')': return make_token(lx, TOK_RPAREN, sp, start, 1);
        case '{': return make_token(lx, TOK_LBRACE, sp, start, 1);
        case '}': return make_token(lx, TOK_RBRACE, sp, start, 1);
        case '[': return make_token(lx, TOK_LBRACK, sp, start, 1);
        case ']': return make_token(lx, TOK_RBRACK, sp, start, 1);
        case ',': return make_token(lx, TOK_COMMA, sp, start, 1);
        case ';': return make_token(lx, TOK_SEMI, sp, start, 1);
        case ':': return make_token(lx, TOK_COLON, sp, start, 1);
        case '.': return make_token(lx, TOK_DOT, sp, start, 1);

        case '+': return make_token(lx, TOK_PLUS, sp, start, 1);
        case '*': return make_token(lx, TOK_STAR, sp, start, 1);
        case '/': return make_token(lx, TOK_SLASH, sp, start, 1);

        case '-':
            if (match(lx, '>')) return make_token(lx, TOK_ARROW, sp, start, 2);
            return make_token(lx, TOK_MINUS, sp, start, 1);

        case '=':
            if (match(lx, '=')) return make_token(lx, TOK_EQEQ, sp, start, 2);
            return make_token(lx, TOK_EQ, sp, start, 1);

        case '!':
            if (match(lx, '=')) return make_token(lx, TOK_UNEQ, sp, start, 2);
            return make_token(lx, TOK_EXCL, sp, start, 1);

        case '<':
            if (match(lx, '=')) return make_token(lx, TOK_LTEQ, sp, start, 2);
            return make_token(lx, TOK_LT, sp, start, 1);

        case '>':
            if (match(lx, '=')) return make_token(lx, TOK_GTEQ, sp, start, 2);
            return make_token(lx, TOK_GT, sp, start, 1);

        default:
            lx->had_error = 1;
            diag_error(sp, "unexpected character '%c' (0x%02x)", (c >= 32 && c < 127) ? c : '?', (unsigned char)c);
            // return something so caller can continue
            return make_token(lx, TOK_EOF, sp, start, 0);
    }
}

const char *token_kind_name(TokenKind k) {
    switch (k) {
        case TOK_EOF: return "EOF";
        case TOK_IDENT: return "IDENT";
        case TOK_INT: return "INT";
        case TOK_STRING: return "STRING";

        case TOK_KW_FN: return "KW_FN";
        case TOK_KW_LET: return "KW_LET";
        case TOK_KW_MUT: return "KW_MUT";
        case TOK_KW_IF: return "KW_IF";
        case TOK_KW_ELSE: return "KW_ELSE";
        case TOK_KW_WHILE: return "KW_WHILE";
        case TOK_KW_RETURN: return "KW_RETURN";
        case TOK_KW_TRUE: return "KW_TRUE";
        case TOK_KW_FALSE: return "KW_FALSE";

        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LBRACK: return "[";
        case TOK_RBRACK: return "]";
        case TOK_COMMA: return ",";
        case TOK_SEMI: return ";";
        case TOK_COLON: return ":";
        case TOK_DOT: return ".";

        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";

        case TOK_EQ: return "=";
        case TOK_EQEQ: return "==";
        case TOK_EXCL: return "!";
        case TOK_UNEQ: return "!=";

        case TOK_LT: return "<";
        case TOK_LTEQ: return "<=";
        case TOK_GT: return ">";
        case TOK_GTEQ: return ">=";

        case TOK_ARROW: return "->";
        default: return "<?>"; 
    }
}