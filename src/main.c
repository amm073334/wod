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
#include "module.h"

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    Arena arena;
    arena_init(&arena);

    StringView input_file = to_sv(argv[1]);

    VEC_Module modules = parse_all_modules(input_file, &arena);
    if (modules.count == 0) exit(2);

    bool success = typecheck_modules(&modules, &arena);
    if (!success) exit(2);

    constexpr_pass(&modules, &arena);

    ast2wir_pass(&modules, &arena);

    GameData gd = wir_pass(&modules, &arena);

    // If an `apply` directory is specified, then apply the text output there.
    // Otherwise, just compile output into the directory the source file is in.
    ProgramAST *main_file_ast = modules.at[0].ast;
    if (sv_is_null(main_file_ast->apply)) {
        StringView directory = get_directory(to_sv(argv[0]), &arena);
        gd_write_dir(&gd, directory);
    } else {
        StringView input_directory = get_directory(input_file, &arena);

        StringView editor_directory = sv_concat(&arena,
            input_directory, main_file_ast->apply);

        char last = main_file_ast->apply.data[main_file_ast->apply.len - 1];
        if (last != '/' && last != '\\')
            editor_directory = sv_concat(&arena,
                editor_directory, SV("\\"));

        StringView build_directory = sv_concat(&arena,
            editor_directory, SV("build"));

        gd_write_dir(&gd, build_directory);
        if (!gd_apply(&arena,
                sv_concat(&arena, editor_directory, SV("Editor.exe")),
                SV("build"))) {
        
            fprintf(stderr, "Failed to apply game data.");
            exit(2);
        }
    }

    arena_free(&arena);

    return 0;
}