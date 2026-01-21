#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s <file.lr>\n", argv0);
}

static int has_lr_extension(const char *path) {
    size_t n = strlen(path);
    return n >= 3 && strcmp(path + n - 3, ".lr") == 0;
}

static void print_sv(StrView s) {
    fwrite(s.ptr, 1, s.len, stdout);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 2;
    }

    const char *path = argv[1];
    if (!has_lr_extension(path)) {
        fprintf(stderr, "%s: error: expected a .lr file\n", path);
        return 2;
    }

    FileBuf fb = read_whole_file(path);
    if (!fb.data) {
        fprintf(stderr, "%s: error: failed to read file\n", path);
        return 1;
    }

    Lexer lx;
    lexer_init(&lx, path, fb.data, fb.len);

    Arena arena;
    arena_init(&arena, 64 * 1024);

    Parser p;
    parser_init(&p, &lx, &arena);

    Program *prog = parse_program(&p);

    if (!prog || p.had_error || lx.had_error) {
        free_filebuf(&fb);
        arena_free(&arena);
        return 1;
    }

    // basic parse summary 
    printf("parsed ok: %zu function(s)\n", prog->fns_len);
    for (size_t i = 0; i < prog->fns_len; i++) {
        FnDecl *fn = prog->fns[i];
        printf("  fn ");
        print_sv(fn->name);
        printf(" (params=%zu) body_stmts=%zu\n", fn->params_len, fn->body_len);
    }

    free_filebuf(&fb);
    arena_free(&arena);
    return 0;
}
