#include "diag.h"
#include <stdio.h>
#include <stdarg.h>



void diag_error(Span where, const char *fmt, ...) {
    fprintf(stderr, "%s:%zu:%zu: error: ", where.path ? where.path : "<stdin>", where.line, where.col);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
}