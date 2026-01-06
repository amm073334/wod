#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "typechecker.h"
#include "ast2wl.h"
#include "gamedata.h"
#include "commonevent.h"

#include "environment.h"

typedef struct {
    Environment modules;
} Compiler;

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

static Environment *typecheck_file(Compiler *compiler, StringView path, Arena *arena) {
    char *source = alloc_source(path, arena);
    VEC_PTR_Stmt *ast = generate_ast(source, arena);
    if (!ast) return NULL;
    
    bool failed = false;
    for (size_t i = 0; i < ast->count; i++) {
        if (ast->at[i]->type != NODE_StmtImport)
            break;

        StringView module = ((StmtImport *)ast->at[i])->path;
        if (!env_get(&compiler->modules, module)) {
            env_insert(&compiler->modules, module, 0, arena);
            Environment *module_env =
                typecheck_file(compiler, module, arena);
            if (!module_env) failed = true;
        }
    }

    if (failed) return NULL;

    return typecheck(ast, source, arena);
}

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    Arena arena;
    arena_init(&arena);

    Compiler compiler;
    env_init(&compiler.modules);

    typecheck_file(&compiler, to_sv(argv[1]), &arena);

    // Arena wl_arena;
    // arena_init(&wl_arena);
    // arena_alloc(&wl_arena, 16384);

    // // TODO: This probably should generate a different set of instructions per cev,
    // //       and compile each of them separately.
    // VEC_WLInst *wl = generate_wl(ast, &wl_arena);

    // Arena gd_arena;
    // CommonEvent cev = compile_wl_to_cev(wl, &gd_arena);

    // GameData gd;
    // gd_init(&gd);
    // VEC_PUSH(gd.cevs, cev, &gd_arena);

    // gd_write_dir(&gd, "build");
    arena_free(&arena);
    return 0;
}