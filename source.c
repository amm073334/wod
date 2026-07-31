#include <stdio.h>

#include "source.h"

Source *alloc_source(StringView path, Arena *arena) {
    char *path_cstr = sv_dup(arena, path);
    Source *out = arena_alloc(arena, sizeof(Source));
    if (!path_cstr || !out) {
        fprintf(stderr, "Not enough memory to read '" SV_FMT "'.\n", SV_FMT_VAL(path));
        return NULL;
    }

    FILE *file = fopen(path_cstr, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file '%s'.\n", path_cstr);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buf = arena_alloc(arena, file_size + 1);
    if (!buf) {
        fprintf(stderr, "Not enough memory to read '%s'.\n", path_cstr);
        return NULL;
    }

    size_t n = fread(buf, sizeof(char), file_size, file);
    if (n < file_size) {
        fprintf(stderr, "Could not read '%s'.\n", path_cstr);
        return NULL;
    }

    buf[n] = '\0';

    fclose(file);
    *out = (Source){
        .text = (StringView){ .data = buf, .len = file_size },
        .path = path
    };

    return out;
}
