#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "typechecker.h"
#include "ast2wir.h"
#include "wir.h"
#include "constexpr.h"
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

    StringView input_file = to_sv(argv[1]);

    char *source = alloc_source(input_file, &arena);
    
    ProgramAST *ast = generate_ast(input_file, source, &arena);
    if (!ast) exit(2);

    Environment *e = typecheck(ast, source, &arena);
    if (!e) exit(2);

    constexpr_pass(ast, &arena);

    // Arena wl_arena;
    // arena_init(&wl_arena);

    // TODO: This probably should generate a different set of instructions per cev,
    //       and compile each of them separately.
    WIR wir = ast2wir_pass(ast, &arena);
    print_wir(&wir);

    // Arena gd_arena;
    // CommonEvent cev = compile_wir_to_cev(wl, &gd_arena);

    // GameData gd;
    // gd_init(&gd);
    // VEC_PUSH(gd.cevs, cev, &gd_arena);

    // gd_write_dir(&gd, "build");
    arena_free(&arena);

    return 0;
}