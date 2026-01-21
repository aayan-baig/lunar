#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "lexer.h"
#include "diag.h"

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s <file.lr>\n", argv0);
}

static void print_token(const Token *t) {
    printf("%zu:%zu  %-8s", t->span.line, t->span.col, token_kind_name(t->kind));

    if (t->kind == TOK_IDENT || t->kind == TOK_STRING) {
        printf("  \"");
        fwrite(t->start, 1, t->length, stdout);
        printf("\"");
    } else if (t->kind == TOK_INT) {
        printf("  %lld", (long long)t->int_val);
    }

    printf("\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 2;
    }

    const char *path = argv[1];
    FileBuf fb = read_whole_file(path);
    if (!fb.data) {
        fprintf(stderr, "%s: error: failed to read file\n", path);
        return 1;
    }

    Lexer lx;
    lexer_init(&lx, path, fb.data, fb.len);

    for (;;) {
        Token t = lexer_next(&lx);
        print_token(&t);
        if (t.kind == TOK_EOF) break;
    }

    free_filebuf(&fb);
    return lx.had_error ? 1 : 0;
}
