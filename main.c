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

    // If an `apply` directory is specified, then apply the text output there.
    // Otherwise, just compile output into the directory the source file is in.
    if (sv_is_null(ast->apply)) {
        StringView directory = get_directory(to_sv(argv[0]), &gd_arena);
        gd_write_dir(&gd, directory);
    } else {
        StringView input_directory = get_directory(input_file, &gd_arena);

        StringView editor_directory = sv_concat(&gd_arena,
            input_directory, ast->apply);

        char last = ast->apply.data[ast->apply.len - 1];
        if (last != '/' && last != '\\')
            editor_directory = sv_concat(&gd_arena,
                editor_directory, SV("\\"));

        StringView build_directory = sv_concat(&gd_arena,
            editor_directory, SV("build"));

        gd_write_dir(&gd, build_directory);
        if (!gd_apply(&gd_arena,
                sv_concat(&gd_arena, editor_directory, SV("Editor.exe")),
                SV("build"))) {
        
            fprintf(stderr, "Failed to apply game data.");
            exit(2);
        }
    }

    arena_free(&arena);
    arena_free(&gd_arena);

    return 0;
}