#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "typechecker.h"
#include "ast2wl.h"
#include "gamedata.h"
#include "commonevent.h"
#include "environment.h"

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    Arena arena;
    arena_init(&arena);

    char *source = alloc_source(to_sv(argv[1]), &arena);
    ProgramAST *ast = generate_ast(to_sv(argv[1]), source, &arena);

    // Environment *e = typecheck(to_sv(argv[1]), &arena);
    // if (!e) exit(2);

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
    // return 0;
}