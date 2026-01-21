#ifndef LUNAR_UTIL_H
#define LUNAR_UTIL_H

#include <stddef.h>

// file buffer struct def
typedef struct {
    char *data;
    size_t len;
} FileBuf;

FileBuf read_whole_file(const char *path);
void free_filebuf(FileBuf *fb);

#endif