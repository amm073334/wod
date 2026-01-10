#include <stdio.h>

#include "typechecker.h"
#include "path.h"
#include "error.h"

typedef struct {
    Environment modules;
    Environment *global_env;
    Environment *current_env;
    int32_t function_offset;
    Arena *arena;
    const char *source;
    bool had_error;
} Typechecker;

static void open_scope(Typechecker *tc) {
    tc->current_env = env_new(tc->current_env, tc->arena);
    if (!tc->current_env) {
        fprintf(stderr, "Fatal error: Out of memory.");
        exit(1);
    }
}

static void close_scope(Typechecker *tc) {
    tc->current_env = tc->current_env->parent;
}

static void tc_error(Typechecker *tc, Token *token, StringView message) {
    tc->had_error = true;
    error(tc->source, token, message);
}

static void globals() {

}

Environment *typecheck(StringView path, Arena *arena) {
    Typechecker tc;
    // tc.current_env = NULL;
    // tc.source = source;
    tc.arena = arena;
    tc.had_error = false;
    tc.function_offset = 0;
    env_init(&tc.modules);

    return typecheck_file(&tc, get_full_path(path, arena), true, arena);
}

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

static Environment *typecheck_file(Typechecker *typechecker, StringView path, bool is_main_file, Arena *arena) {
    char *source = alloc_source(path, arena);
    VEC_PTR_Stmt *ast = generate_ast(source, arena);
    if (!ast) return NULL;
    
    Environment imports;
    env_init(&imports);
    
    Environment *globals = arena_alloc(arena, sizeof(Environment));
    if (!globals) {
        fprintf(stderr, "Could not allocate globals.");
        return NULL;
    }
    env_init(globals);

    // Handle imports.
    bool import_failed = false;
    size_t i = 0;
    for (; i < ast->count; i++) {
        if (ast->at[i]->type != NODE_StmtImport)
            break;

        StringView module_path =
            get_full_path(((StmtImport *)ast->at[i])->path, arena);
        Environment *module_env;

        // Only check each file once.
        Symbol *search = env_find(&typechecker->modules, module_path);
        if (search) {
            module_env = search->type.env;
        } else {
            Symbol *sym = env_insert(&typechecker->modules, module_path, TYPE_MODULE, 0, arena);
            assert(sym);

            module_env = typecheck_file(typechecker, module_path, false, arena);
            if (!module_env) import_failed = true;
        
            sym->type.env = module_env;
        }

        Symbol *sym = env_insert(&imports, module_path, TYPE_MODULE, 0, arena);
        if (sym) sym->type.env = module_env;
    }

    if (import_failed) return NULL;

    // Compile the rest of the statements.
    for (; i < ast->count; i++) {
        Stmt *stmt = ast->at[i];
        switch (stmt->type) {
            case NODE_StmtVarDecl: {
                StmtVarDecl *var = (StmtVarDecl *)stmt;

                BaseType type;
                switch (var->type.type) {
                    case TOK_INT:
                        type = TYPE_INT;
                        break;
                    case TOK_STR:
                        type = TYPE_STR;
                        break;
                    case TOK_BOOL:
                        type = TYPE_BOOL;
                        break;
                    default: UNREACHABLE;
                }

                Symbol *sym = env_insert(globals, var->name, type,
                                0, arena);
                if (!sym)
                    tc_error(typechecker, &var->base.loc,
                        SV("Redeclaration of name."));
                break;
            }
            case NODE_StmtFuncDecl: {
                StmtFuncDecl *func = (StmtFuncDecl *)stmt;
                
                Symbol *sym = env_insert(globals, func->name, TYPE_FUNC,
                                typechecker->function_offset++, arena);
                if (!sym)
                    tc_error(typechecker, &func->base.loc,
                        SV("Redeclaration of name."));

                switch (func->ret.type) {
                    case TOK_VOID:
                        sym->type.return_type = TYPE_VOID;
                        break;
                    case TOK_INT:
                        sym->type.return_type = TYPE_INT;
                        break;
                    case TOK_STR:
                        sym->type.return_type = TYPE_STR;
                        break;
                    case TOK_BOOL:
                        sym->type.return_type = TYPE_BOOL;
                        break;
                    default: UNREACHABLE;
                }

                for (size_t param = 0; param < func->params.count; param++) {
                    switch (func->params.at[param]->type.type) {
                        case TOK_INT:
                            VEC_PUSH(sym->type.params,
                                (WodType){TYPE_INT}, arena);
                            break;
                        case TOK_STR:
                            VEC_PUSH(sym->type.params,
                                (WodType){TYPE_STR}, arena);
                            break;
                        case TOK_BOOL:
                            VEC_PUSH(sym->type.params,
                                (WodType){TYPE_BOOL}, arena);
                            break;
                        default: UNREACHABLE;
                    }
                }

                break;
            }
            default: UNREACHABLE;
        }
    }

    if (is_main_file && !env_find(globals, SV("main")))
        tc_error(&typechecker, NULL, SV("No 'main' function."));

    return typechecker->had_error ? NULL : globals;
}