#ifndef LUNAR_DIAG_H
#define LUNAR_DIAG_H

#include <stddef.h>

// Span of file path and a line and col in a struct literally called span lol

typedef struct {
    const char *path;
    size_t line;
    size_t col;
} Span;


void diag_error(Span where, const char *fmt, ...);

#endif