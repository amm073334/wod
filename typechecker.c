#include <stdio.h>

#include "typechecker.h"
#include "error.h"

typedef struct {
    Environment *global_env;
    Environment *current_env;
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

Environment *typecheck(VEC_PTR_Stmt *ast, const char *source, Arena *arena) {
    Typechecker tc;
    tc.current_env = NULL;
    tc.arena = arena;
    tc.source = source;
    tc.had_error = false;

    open_scope(&tc);
    tc.global_env = tc.current_env;
    
    size_t i = 0;
    for (; i < ast->count; i++) {
        if (ast->at[i]->type != NODE_StmtImport)
            break;
        // TODO: imports
    }

    bool found_main = false;
    for (; i < ast->count; i++) {
        Stmt *stmt = ast->at[i];
        switch (stmt->type) {
            case NODE_StmtVarDecl:
                break;
            case NODE_StmtFuncDecl:
                if (sv_equals(((StmtFuncDecl *)stmt)->name, SV("main")))
                    found_main = true;
                break;
            default: UNREACHABLE;
        }
    }

    if (!found_main)
        tc_error(&tc, NULL, SV("No 'main' function."));

    close_scope(&tc);
    assert(!tc.current_env);

    if (tc.had_error) return NULL;
    return tc.global_env;
}