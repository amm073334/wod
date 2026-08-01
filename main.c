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

static bool parse_all_modules_helper(
    VEC_Module *modules, 
    StringView path, 
    Arena *arena, 
    Import *import
) {
    // Canonicalize import paths.
    if (import) import->path = path;

    // If file has already been parsed, do nothing.
    for (size_t i = 0; i < modules->count; i++) {
        if (sv_equals(path, modules->at[i].source->path)) {
            return true;
        }
    }
    
    // Otherwise recursively parse the file.
    Source *sub_source = alloc_source(path, arena);
    if (!sub_source) return false;

    ProgramAST *ast = generate_ast(*sub_source, arena);
    if (!ast) return false;

    VEC_PUSH(*modules,
        ((Module){ .source = sub_source, .ast = ast }), arena);

    StringView dir = get_directory(path, arena);
    if (sv_is_null(dir)) return false;

    bool success = true;
    for (size_t i = 0; i < ast->imports.count; i++) {
        Import *import = &ast->imports.at[i];
        
        StringView absolute_import_path;
        {
            char *import_path = sv_dup(arena, import->path);
            if (path_is_relative(import_path)) {
                absolute_import_path = sv_concat(arena, 
                    dir, to_sv(import_path));
            } else {
                absolute_import_path = to_sv(import_path);
            }
        }

        if (sv_is_null(absolute_import_path))
            return false;

        success = success && 
            parse_all_modules_helper(modules, absolute_import_path, arena, import);
    }

    return success;
}

static VEC_Module parse_all_modules(StringView path, Arena *arena) {
    VEC_Module modules = VEC_EMPTY;

    StringView full_path = get_full_path(path, arena);
    if (sv_is_null(full_path)) return (VEC_Module)VEC_EMPTY;

    bool success = parse_all_modules_helper(&modules, full_path, arena, NULL);

    return success ? modules : (VEC_Module)VEC_EMPTY;
}

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

    // TODO: This probably should generate a different set of instructions per cev,
    //       and compile each of them separately.
    ast2wir_pass(&modules, &arena);
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