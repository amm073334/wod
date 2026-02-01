#include <stdio.h>

#include "sm.h"
#include "parser.h"
#include "error.h"
#include "environment.h"

typedef struct {
    char *source;
    Arena *arena;

    Environment env;
    StmtSMState *state;
    StmtSMDecl *sm;
} SMChecker;

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

static bool expr_match(SMChecker *smc, Expr *expr, Expr *pattern) {
    if (expr->type != pattern->type) return false;

    switch (expr->type) {
        case NODE_ExprVar:
            UNIMPLEMENTED;
        case NODE_ExprArray:
            UNIMPLEMENTED;
        case NODE_ExprDB:
            UNIMPLEMENTED;
        case NODE_ExprBinary:
            UNIMPLEMENTED;
        case NODE_ExprUnary:
            UNIMPLEMENTED;
        case NODE_ExprCall: {
            ExprCall *exp = (ExprCall *)expr;
            ExprCall *pat = (ExprCall *)pattern;
            if (!sv_equals(exp->name, pat->name))
                return false;
            if (exp->args.count != pat->args.count)
                return false;
            for (size_t i = 0; i < exp->args.count; i++) {
                if (!expr_match(smc, exp->args.at[i], pat->args.at[i]))
                    return false;
            }
            return true;
        }
        case NODE_ExprIntLit:
            UNIMPLEMENTED;
        case NODE_ExprStrLit:
            UNIMPLEMENTED;
        case NODE_ExprSMMatch:
            UNIMPLEMENTED;
        default: UNREACHABLE;
    }
    return false;
}

static bool visit_Expr(SMChecker *smc, Expr *stmt);
static bool visit_Stmt(SMChecker *smc, Stmt *stmt);

static bool visit_param(SMChecker *smc, StmtVarDecl *param) {
    return false;
}


static bool visit_StmtFuncDecl(SMChecker *smc, StmtFuncDecl *stmt) {
    bool success = true;
    for (size_t i = 0; i < stmt->body.count; i++)
        success = success && visit_Stmt(smc, stmt->body.at[i]);
    return success;
}

static bool visit_ExprCall(SMChecker *smc, ExprCall *call) {
    bool success = true;
    for (size_t i = 0; i < call->args.count; i++)
        success = success && visit_Expr(smc, call->args.at[i]);
    return success;
}

static bool visit_StmtExpr(SMChecker *smc, StmtExpr *stmt) {
    return visit_Expr(smc, stmt->expr);
}

static void sm_error(SMChecker *smc, Token *tok, VEC_PTR_Stmt action) {
    for (size_t i = 0; i < action.count; i++) {
        if (action.at[i]->type != NODE_StmtExpr) {
            fprintf(stderr, "Unknown action.\n");
            return;
        }

        StmtExpr *se = ((StmtExpr *)action.at[i]);
        if (se->expr->type != NODE_ExprCall) {
            fprintf(stderr, "Unknown action.\n");
            return;
        }

        ExprCall *ec = (ExprCall *)se->expr;
        if (sv_equals(ec->name, SV("err"))) {
            if (ec->args.count == 1 
                && ec->args.at[0]->type == NODE_ExprStrLit) {
                error(smc->source, tok, 
                    ((ExprStrLit *)ec->args.at[0])->value);
            } else {
                error(smc->source, tok, SV("Error."));
            }
            return;                
        }
    }
    fprintf(stderr, "Unknown action.\n");
    return;
}

static bool visit_Expr(SMChecker *smc, Expr *expr) {
    VEC_PTR_ExprSMMatch matches = smc->state->matches;
    for (size_t i = 0; i < matches.count; i++) {
        if (!expr_match(smc, expr, matches.at[i]->expr_pattern))
            continue;
        
        if (sv_equals(matches.at[i]->next_state, SV(""))) {
            sm_error(smc, &expr->loc, matches.at[i]->action);
            return false;
        }

        Symbol *sym = env_find(&smc->env, matches.at[i]->next_state);
        assert(sym && sym->type.basetype == TYPE_SM_STATE);

        smc->state = (StmtSMState *)smc->sm->body.at[sym->offset];
        return true;
    }
    return true;
}

static bool visit_Stmt(SMChecker *smc, Stmt *stmt) {
    switch (stmt->type) {
        case NODE_StmtImport:
            UNIMPLEMENTED;
        case NODE_StmtAssign:
            UNIMPLEMENTED;
        case NODE_StmtVarDecl:
            UNIMPLEMENTED;
        case NODE_StmtFuncDecl:
            return visit_StmtFuncDecl(smc, (StmtFuncDecl *)stmt);
        case NODE_StmtBlock:
            UNIMPLEMENTED;
        case NODE_StmtReturn:
            UNIMPLEMENTED;
        case NODE_StmtIf:
            UNIMPLEMENTED;
        case NODE_StmtLoop:
            UNIMPLEMENTED;
        case NODE_StmtFor:
            UNIMPLEMENTED;
        case NODE_StmtContinue:
            UNIMPLEMENTED;
        case NODE_StmtBreak:
            UNIMPLEMENTED;
        case NODE_StmtCmd:
            UNIMPLEMENTED;
        case NODE_StmtDBDecl:
            UNIMPLEMENTED;
        case NODE_StmtExpr:
            return visit_StmtExpr(smc, (StmtExpr *)stmt);
        
        case NODE_StmtSMDecl:
        case NODE_StmtSMState:
            return true;
        default: UNREACHABLE;
    }
    return false;
}

static const int TYPE_TABLE[] = {
    [TOK_ANY] = TYPE_SM_ANY,
    [TOK_ANY_ARGS] = TYPE_SM_ANY_ARGS,
    [TOK_ANY_CALL] = TYPE_SM_ANY_CALL,
};

static bool run_sm(SMChecker *smc, VEC_PTR_Stmt *ast, StmtSMDecl *sm) {
    env_init(&smc->env);
    smc->sm = sm;

    bool failed = false;
    size_t i = 0;
    
    for (; i < sm->body.count
           && sm->body.at[i]->type == NODE_StmtSMDecl; i++) {
        StmtVarDecl *decl = (StmtVarDecl *)sm->body.at[i];

        assert(decl->type.type < ARRLEN(TYPE_TABLE));
        Symbol *sym = env_insert(&smc->env, decl->name,
            TYPE_TABLE[decl->type.type], 0, smc->arena);

        if (!sym) {
            failed = true;
            error(smc->source, &decl->base.loc,
                SV("Failed to insert declaration."));
        }
    }
    if (failed) return false;

    // If there are no states.
    if (i >= sm->body.count)
        return true;
    
    smc->state = (StmtSMState *)sm->body.at[i];
    for (; i < sm->body.count; i++) {
        StmtSMState *state = (StmtSMState *)sm->body.at[i];

        Symbol *sym = env_insert(&smc->env, state->name,
            TYPE_SM_STATE, i, smc->arena);

        if (!sym) {
            failed = true;
            error(smc->source, &state->base.loc,
                SV("Failed to insert state."));
        }
    }
    if (failed) return false;

    for (size_t stmt = 0; stmt < ast->count; stmt++)
        if (!visit_Stmt(smc, ast->at[stmt])) return false;

    return true;
}

bool run_sm_checker(StringView path, Arena *arena) {
    char *source = alloc_source(path, arena);
    VEC_PTR_Stmt *ast = generate_ast(source, arena);
    if (!ast) return false;

    SMChecker smc;
    smc.source = source;
    smc.arena = arena;

    bool failed = false;
    for (size_t i = 0; i < ast->count; i++) {
        if (ast->at[i]->type == NODE_StmtSMDecl) {
            bool result = run_sm(&smc, ast, (StmtSMDecl *)ast->at[i]);
            if (!result)
                failed = true;
        }
    }

    return !failed;
}