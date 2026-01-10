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
    // Table of modules compiled for the entire program so far.
    Environment modules;

    int32_t function_offset;
} Compiler;

static Symbol *find(Environment *env, Environment *imports, StringView name) {
    Symbol *search = env_find(env, name);
    if (search) return search;

    for (size_t i = 0; i < imports->symbols.count; i++) {
        search = env_find(imports->symbols.at[i].type.env, name);
        if (search) return search;
    }

    return NULL;
}


int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    Arena arena;
    arena_init(&arena);

    typecheck(to_sv(argv[1]), &arena);

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