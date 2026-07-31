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

    // StringView full_path = get_full_path(input_file, &arena);
    // StringView dir = get_directory(full_path, &arena);
    // printf("test: " SV_FMT "\n", SV_FMT_VAL(full_path));
    // printf("test: " SV_FMT "\n", SV_FMT_VAL(dir));
    // return 0;

    VEC_PTR_ProgramAST asts = generate_all_asts(input_file, &arena);
    if (asts.count == 0) exit(2);

    // VEC_PTR_Environment e = typecheck_asts(asts, &arena);
    // if (e.count == 0) exit(2);

    // constexpr_pass(ast, &arena);

    // TODO: This probably should generate a different set of instructions per cev,
    //       and compile each of them separately.
    // WIR wir = ast2wir_pass(ast, &arena);
    // print_wir(&wir);

    // Arena gd_arena;
    // arena_init(&gd_arena);

    // GameData gd = compile_wir_to_gd(wir, &gd_arena);

    // If an `apply` directory is specified, then apply the text output there.
    // Otherwise, just compile output into the directory the source file is in.
    // if (sv_is_null(ast->apply)) {
    //     StringView directory = get_directory(to_sv(argv[0]), &gd_arena);
    //     gd_write_dir(&gd, directory);
    // } else {
    //     StringView input_directory = get_directory(input_file, &gd_arena);

    //     StringView editor_directory = sv_concat(&gd_arena,
    //         input_directory, ast->apply);

    //     char last = ast->apply.data[ast->apply.len - 1];
    //     if (last != '/' && last != '\\')
    //         editor_directory = sv_concat(&gd_arena,
    //             editor_directory, SV("\\"));

    //     StringView build_directory = sv_concat(&gd_arena,
    //         editor_directory, SV("build"));

    //     gd_write_dir(&gd, build_directory);
    //     if (!gd_apply(&gd_arena,
    //             sv_concat(&gd_arena, editor_directory, SV("Editor.exe")),
    //             SV("build"))) {
        
    //         fprintf(stderr, "Failed to apply game data.");
    //         exit(2);
    //     }
    // }

    arena_free(&arena);
    // arena_free(&gd_arena);

    return 0;
}