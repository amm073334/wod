#include <stdio.h>

#include "sm.h"
#include "parser.h"

static char *alloc_source(StringView path, Arena *arena) {
    char *path_cstr = arena_alloc(arena, path.len + 1);
    if (!path_cstr) {
        fprintf(stderr, "Not enough memory to read '" SV_FMT "'.", SV_FMT_VAL(path));
        exit(1);
    }
    memcpy(path_cstr, path.data, path.len);
    path_cstr[path.len] = '\0';

    FILE *file = fopen(path_cstr, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file '%s'.", path_cstr);
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buf = arena_alloc(arena, file_size + 1);
    if (!buf) {
        fprintf(stderr, "Not enough memory to read '%s'.", path_cstr);
        exit(1);
    }

    size_t n = fread(buf, sizeof(char), file_size, file);
    if (n < file_size) {
        fprintf(stderr, "Could not read '%s'.", path_cstr);
        exit(1);
    }

    buf[n] = '\0';

    fclose(file);
    return buf;
}

static bool run_sm(VEC_PTR_Stmt *ast, StmtSMDecl *sm) {
    return true;
}

bool run_sm_checker(StringView path, Arena *arena) {
    char *source = alloc_source(path, arena);
    VEC_PTR_Stmt *ast = generate_ast(source, arena);
    if (!ast) return false;

    bool failed = false;
    for (size_t i = 0; i < ast->count; i++) {
        if (ast->at[i]->type == NODE_StmtSMDecl) {
            bool result = run_sm(ast, (StmtSMDecl *)ast->at[i]);
            if (!result)
                failed = true;
        }
    }

    return !failed;
}