#include "util.h"
#include <stdio.h>
#include <stdlib.h>

FileBuf read_whole_file(const char *path) {
    FileBuf out = {0};

    FILE *f = fopen(path, "rb");
    if(!f) return out;

    // determine file size
    if(fseek(f,0,SEEK_END)!=0) {fclose(f); return out;}
    long sz = ftell(f);
    if(sz<0){fclose(f); return out;}
    if(fseek(f,0,SEEK_SET)!=0){fclose(f); return out;}

    //malloc for contents
    out.data = (char *)malloc((size_t)sz+1);
    if(!out.data) {fclose(f); return(FileBuf){0};}

    //read into mem
    size_t n = fread(out.data,1,(size_t)sz,f);
    fclose(f);

    //null term && set len
    out.data[n] = '\0';
    out.len = n;

    // returns filebuf w/ file size & data
    return out;
}

void free_filebuf(FileBuf *fb) {
    // rather easy funct, has to free allocated mem.
    if (!fb) return;

    free(fb->data);
    fb->data = NULL;
    fb->len = 0; /* full field reset */
}