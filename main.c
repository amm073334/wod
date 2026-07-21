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
#include "path.h"

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

    // TODO: This probably should generate a different set of instructions per cev,
    //       and compile each of them separately.
    WIR wir = ast2wir_pass(ast, &arena);
    print_wir(&wir);

    Arena gd_arena;
    arena_init(&gd_arena);

    GameData gd = compile_wir_to_gd(wir, &gd_arena);

    for (size_t i = 0; i < gd.cevs.count; i++) {
        cev_write_txt(&gd.cevs.at[i], stdout);
        printf("\n");
    }

    if (sv_is_null(ast->apply)) {
        StringView directory = get_directory(to_sv(argv[0]), &gd_arena);
        gd_write_dir(&gd, directory);
    } else {
        StringView directory = get_directory(input_file, &gd_arena);
        gd_write_dir(&gd, sv_concat(&gd_arena, directory, ast->apply));
    }

    arena_free(&arena);
    arena_free(&gd_arena);

    return 0;
}