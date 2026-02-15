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

    StringView file_path;
} SMChecker;

static bool expr_match(SMChecker *smc, Expr *expr, Expr *pattern) {
    if (pattern->type == NODE_ExprVar) {
        ExprVar *pat = (ExprVar *)pattern;
        Symbol *sym = env_find(&smc->env, pat->name);
        if (sym && sym->type.basetype == TYPE_SM_ANY) {
            if (sym->value == NULL) {
                sym->value = expr;
                return true;
            } else {
                return expr_match(smc, expr, (Expr *)sym->value);
            }
        }
    }
    
    if (expr->type != pattern->type) return false;

    switch (expr->type) {
        case NODE_ExprVar:
            UNIMPLEMENTED;
        case NODE_ExprArray:
            UNIMPLEMENTED;
        case NODE_ExprDB:
            UNIMPLEMENTED;
        case NODE_ExprBinary: {
            ExprBinary *exp = (ExprBinary *)expr;
            ExprBinary *pat = (ExprBinary *)pattern;

            return exp->op.type == pat->op.type
                && expr_match(smc, exp->left, pat->left)
                && expr_match(smc, exp->right, pat->right);
        }
        case NODE_ExprUnary:
            UNIMPLEMENTED;
        case NODE_ExprCall: {
            ExprCall *exp = (ExprCall *)expr;
            ExprCall *pat = (ExprCall *)pattern;

            // Handle any_call.
            {
                Symbol *sym = env_find(&smc->env, pat->name);

                // NULL value means variable hasn't been bound yet.
                if (sym && sym->type.basetype == TYPE_SM_ANY_CALL) {
                    if (sym->value == NULL) {
                        sym->value = exp;
                    } else {
                        return expr_match(smc, expr, (Expr *)sym->value);
                    }
                } else if (!sv_equals(exp->name, pat->name))
                    return false;
            }

            // Handle any_args; only work if call has exactly one arg.
            {
                if (pat->args.count == 1
                    && pat->args.at[0]->type == NODE_ExprVar) {

                    Symbol *sym =
                        env_find(&smc->env, ((ExprVar *)pat->args.at[0])->name);

                    if (sym && sym->type.basetype == TYPE_SM_ANY_ARGS) {
                        if (sym->value == NULL) {
                            // Since there is no AST node for params, bind the
                            // variable to the call instead.
                            sym->value = exp;
                            return true;
                        } else {
                            // If args already bound, then fall through to
                            // check that all args match.
                            pat = (ExprCall *)sym->value;
                        }
                    }
                }
            }

            if (exp->args.count != pat->args.count)
                return false;
            for (size_t i = 0; i < exp->args.count; i++) {
                if (!expr_match(smc, exp->args.at[i], pat->args.at[i]))
                    return false;
            }
            return true;
        }
        case NODE_ExprIntLit: {
            ExprIntLit *exp = (ExprIntLit *)expr;
            ExprIntLit *pat = (ExprIntLit *)pattern;

            return exp->value == pat->value;
        }
        case NODE_ExprStrLit:
            UNIMPLEMENTED;

        case NODE_ExprSMMatch:
        default: UNREACHABLE;
    }
    return false;
}

static bool visit_Expr(SMChecker *smc, Expr *expr);
static bool visit_Stmt(SMChecker *smc, Stmt *stmt);

static bool visit_args(SMChecker *smc, StmtVarDecl *param) {
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

static bool sm_func_action(SMChecker *smc, Token *tok, ExprCall *action) {
    if (sv_equals(action->name, SV("err"))) {
        if (action->args.count == 1 
            && action->args.at[0]->type == NODE_ExprStrLit) {
            
            error(smc->file_path, smc->source, tok, 
                ((ExprStrLit *)action->args.at[0])->value);
        } else {
            error(smc->file_path, smc->source, &action->base.loc,
                SV("Bad action."));
        }
        return false;
    } else if (sv_equals(action->name, SV("mgk_expr_recurse"))) {
        if (!(action->args.count == 2
            && action->args.at[0]->type == NODE_ExprVar
            && action->args.at[1]->type == NODE_ExprVar)) {

            
            error(smc->file_path, smc->source, &action->base.loc,
                SV("Bad action."));
            return false;
        }

        StringView state_name = ((ExprVar *)action->args.at[1])->name;
        Symbol *sym = env_find(&smc->env, state_name);

        if (!sym) {
            error(smc->file_path, smc->source, &action->base.loc,
                SV("Bad action."));
            return false;
        }

        SMChecker smc_copy = *smc;
        smc_copy.state = (StmtSMState *)sym->value;

        Expr *next_node = (Expr *)env_find(&smc->env,
            ((ExprVar *)action->args.at[0])->name)->value;

        return visit_Expr(&smc_copy, next_node);
    } else {
        fprintf(stderr, "Unknown action.\n");
        return false;
    }
}

static bool sm_action(SMChecker *smc, Token *tok, VEC_PTR_Stmt action) {
    assert(action.count == 1);
    
    if (action.at[0]->type != NODE_StmtExpr) {
        fprintf(stderr, "Unknown action.\n");
        return false;
    }

    StmtExpr *se = ((StmtExpr *)action.at[0]);
    if (se->expr->type != NODE_ExprCall) {
        fprintf(stderr, "Unknown action.\n");
        return false;
    }

    return sm_func_action(smc, tok, (ExprCall *)se->expr);
}

// Returns false if error, true if successful or no relevant states.
static bool transition(SMChecker *smc, Expr *expr) {
    VEC_PTR_ExprSMMatch matches = smc->state->matches;
    for (size_t i = 0; i < matches.count; i++) {
        // Reset all bound variables before a transition is made.
        // (A variable bound during one transition shouldn't persist to
        //  other transitions, nor should it persist if the variable was
        //  bound but the transition didn't match completely.)
        for (size_t sym = 0; sym < smc->env.symbols.count; sym++) {
            if (smc->env.symbols.at[sym].type.basetype != TYPE_SM_STATE)
                smc->env.symbols.at[sym].value = NULL;
        }

        if (!expr_match(smc, expr, matches.at[i]->expr_pattern))
            continue;
        
        if (sv_equals(matches.at[i]->next_state, SV(""))) {
            if (!sm_action(smc, &expr->loc, matches.at[i]->action))
                return false;
            else continue;
        }

        Symbol *sym = env_find(&smc->env, matches.at[i]->next_state);
        assert(sym && sym->type.basetype == TYPE_SM_STATE);

        smc->state = (StmtSMState *)sym->value;
        return true;
    }
    return true;
}

static bool visit_Expr(SMChecker *smc, Expr *expr) {
    if (!transition(smc, expr)) return false;

    bool success = true;
    switch (expr->type) {
        case NODE_ExprVar:
            UNIMPLEMENTED;
        case NODE_ExprArray:
            UNIMPLEMENTED;
        case NODE_ExprDB:
            UNIMPLEMENTED;
        case NODE_ExprBinary: {
            ExprBinary *e = (ExprBinary *)expr;
            success = success && visit_Expr(smc, e->left);
            success = success && visit_Expr(smc, e->right);
            return success;
        }
        case NODE_ExprUnary: {
            ExprUnary *e = (ExprUnary *)expr;
            success = success && visit_Expr(smc, e->right);
            return success;
        }
        case NODE_ExprCall: {
            ExprCall *e = (ExprCall *)expr;
            for (size_t i = 0; i < e->args.count; i++)
                success = success && visit_Expr(smc, e->args.at[i]);
            return success;
        }
        case NODE_ExprIntLit:
            return success;
        case NODE_ExprStrLit:
            return success;

        case NODE_ExprSMMatch:
        default:
            UNREACHABLE;
    }
    return false;
}

static bool split_SM(SMChecker *base_smc, Stmt *stmt) {

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
            if (!smc->sm->flow_insensitive)
                
            UNIMPLEMENTED;
        case NODE_StmtIf: {
            StmtIf *st = (StmtIf *)stmt;
            bool res = visit_Expr(smc, st->condition);
            if (smc->sm->flow_insensitive)
                return res;

            res &= visit_Stmt(smc, st->then_branch);
            SMChecker smc_copy = *smc;

            if (st->else_branch)
                res &= visit_Stmt(&smc_copy, st->else_branch);

            // TODO: this doesn't quite work: the smc copy is going
            // to return after it visits the branch, when it needs
            // to keep going after it visits the branch
        }
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
           && sm->body.at[i]->type == NODE_StmtVarDecl; i++) {
        StmtVarDecl *decl = (StmtVarDecl *)sm->body.at[i];

        assert(decl->type.type < ARRLEN(TYPE_TABLE));
        Symbol *sym = env_insert(&smc->env, decl->name,
            TYPE_TABLE[decl->type.type], 0, smc->arena);

        if (!sym) {
            failed = true;
            error(smc->file_path, smc->source, &decl->base.loc,
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
            TYPE_SM_STATE, state, smc->arena);

        if (!sym) {
            failed = true;
            error(smc->file_path, smc->source, &state->base.loc,
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
    VEC_PTR_Stmt *ast = generate_ast(path, source, arena);
    if (!ast) return false;

    SMChecker smc;
    smc.source = source;
    smc.arena = arena;
    smc.file_path = path;

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