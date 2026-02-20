#include <stdio.h>

#include "sm.h"
#include "parser.h"
#include "cfg.h"
#include "error.h"
#include "environment.h"

typedef struct {
    bool had_error;

    char *source;
    Arena *arena;

    Environment env;
    StmtSMState *state;
    StmtSMDecl *sm;

    CFGNode *current_node;
    size_t stmt_in_node;

    // Kind of a hack. Stores the most-recently seen
    // token, for error reporting.
    Token last;

    // Another hack. Stores the most-recently-seen
    StmtSMState *false_next;

    StringView file_path;
} SMChecker;

typedef struct {
    // AST node to bind a variable to.
    void *match;

    // For variables that store state.
    bool has_state;
    StmtSMState *state;
} SMVar;

void smc_error(SMChecker *smc, Token *token, StringView message) {
    smc->had_error = true;
    error(smc->file_path, smc->source, token, message);
}


// Copy SMChecker, and duplicate all SMVar objects in its environment.
static SMChecker smc_copy(SMChecker *smc, Arena *arena) {
    SMChecker copy = *smc;
    copy.env.symbols.at 
        = arena_alloc(arena, copy.env.symbols.capacity * sizeof(Symbol));
    assert(copy.env.symbols.at);

    memcpy(copy.env.symbols.at, smc->env.symbols.at,
        smc->env.symbols.count * sizeof(Symbol));

    for (size_t i = 0; i < copy.env.symbols.count; i++) {
        if (copy.env.symbols.at[i].type.basetype != TYPE_SM_STATE) {
            
            copy.env.symbols.at[i].value
                = arena_alloc(arena, sizeof(SMVar));
            assert(copy.env.symbols.at[i].value);
            
            memcpy(copy.env.symbols.at[i].value,
                 smc->env.symbols.at[i].value, sizeof(SMVar));
        }
    }

    return copy;
}

static bool expr_match(SMChecker *smc, Expr *expr, Expr *pattern) {
    if (pattern->type == NODE_ExprVar) {
        ExprVar *pat = (ExprVar *)pattern;
        Symbol *sym = env_find(&smc->env, pat->name);
        if (sym && sym->type.basetype == TYPE_SM_ANY) {
            SMVar *v = (SMVar *)sym->value;
            if (v->match == NULL) {
                v->match = expr;
                return true;
            } else {
                return expr_match(smc, expr, (Expr *)v->match);
            }
        }
    }
    
    if (expr->type != pattern->type) return false;

    switch (expr->type) {
        case NODE_ExprVar: {
            ExprVar *exp = (ExprVar *)expr;
            ExprVar *pat = (ExprVar *)pattern;
            return sv_equals(exp->name, pat->name);
        }
        case NODE_ExprArray: {
            ExprArray *exp = (ExprArray *)expr;
            ExprArray *pat = (ExprArray *)pattern;
            
            return expr_match(smc, exp->left, pat->left)
                && expr_match(smc, exp->index, pat->index);
        }
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
                // If pattern is a simple call, it could be
                // a SM variable: search for it.
                Symbol *sym = NULL;
                if (pat->callee->type == NODE_ExprVar) {
                    sym = env_find(&smc->env, 
                        ((ExprVar *)pat->callee)->name);
                }

                // NULL value means variable hasn't been bound yet.
                if (sym && sym->type.basetype == TYPE_SM_ANY_CALL) {
                    SMVar *v = ((SMVar *)sym->value);
                    if (v->match == NULL) {
                        v->match = exp;
                    } else {
                        return expr_match(smc, expr, (Expr *)v->match);
                    }
                } else if (!expr_match(smc, exp->callee, pat->callee))
                    return false;
            }

            // Handle any_args; only work if call has exactly one arg.
            {
                if (pat->args.count == 1
                    && pat->args.at[0]->type == NODE_ExprVar) {

                    Symbol *sym =
                        env_find(&smc->env, ((ExprVar *)pat->args.at[0])->name);

                    if (sym && sym->type.basetype == TYPE_SM_ANY_ARGS) {
                        SMVar *v = ((SMVar *)sym->value);
                        if (v->match == NULL) {
                            // Since there is no AST node for params, bind the
                            // variable to the call instead.
                            v->match = exp;
                            return true;
                        } else {
                            // If args already bound, then fall through to
                            // check that all args match.
                            pat = (ExprCall *)v->match;
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

        case NODE_ExprSMEndOfPath:
            return true;

        case NODE_ExprSMMatch:
        default: UNREACHABLE;
    }
    return false;
}

static void visit_Expr(SMChecker *smc, Expr *expr);
static void visit_Stmt(SMChecker *smc, Stmt *stmt);

static bool visit_args(SMChecker *smc, StmtVarDecl *param) {
    return false;
}

static void sm_func_action(SMChecker *smc, Token *tok, ExprCall *action) {
    // Only handle simple call actions.
    if (action->callee->type != NODE_ExprVar)
        return;

    StringView action_name = ((ExprVar *)action->callee)->name;

    if (sv_equals(action_name, SV("err"))) {
        if (action->args.count == 1 
            && action->args.at[0]->type == NODE_ExprStrLit) {
            
            smc_error(smc, tok, 
                ((ExprStrLit *)action->args.at[0])->value);
        } else {
            smc_error(smc, &action->base.loc,
                SV("Bad action."));
        }
        return;
    } else if (sv_equals(action_name, SV("mgk_expr_recurse"))) {
        if (!(action->args.count == 2
            && action->args.at[0]->type == NODE_ExprVar
            && action->args.at[1]->type == NODE_ExprVar)) {

            
            smc_error(smc, &action->base.loc,
                SV("Bad action."));
            return;
        }

        StringView state_name = ((ExprVar *)action->args.at[1])->name;
        Symbol *sym = env_find(&smc->env, state_name);

        if (!sym) {
            smc_error(smc, &action->base.loc,
                SV("Bad action."));
            return;
        }

        SMChecker copy = smc_copy(smc, smc->arena);
        copy.state = (StmtSMState *)sym->value;

        Expr *next_node = ((SMVar *)(env_find(&smc->env,
            ((ExprVar *)action->args.at[0])->name)->value))->match;

        visit_Expr(&copy, next_node);

        if (copy.had_error)
            smc->had_error = true;
        return;
    } else {
        fprintf(stderr, "Unknown action.\n");
        smc->had_error = true;
        return;
    }
}

static void sm_action(SMChecker *smc, Token *tok, VEC_PTR_Stmt action) {
    assert(action.count == 1);
    
    if (action.at[0]->type != NODE_StmtExpr) {
        fprintf(stderr, "Unknown action.\n");
        smc->had_error = true;
        return;
    }

    StmtExpr *se = ((StmtExpr *)action.at[0]);
    if (se->expr->type != NODE_ExprCall) {
        fprintf(stderr, "Unknown action.\n");
        smc->had_error = true;
        return;
    }

    sm_func_action(smc, tok, (ExprCall *)se->expr);
}

// Find first transition in list that matches expr. Return true if found.
static bool check_matches(SMChecker *smc, Expr *expr, VEC_PTR_ExprSMMatch matches) {
    for (size_t i = 0; i < matches.count; i++) {
        // Reset all bound variables before a transition is made.
        // (A variable bound during one transition shouldn't persist to
        //  other transitions, nor should it persist if the variable was
        //  bound but the transition didn't match completely.)
        for (size_t sym = 0; sym < smc->env.symbols.count; sym++) {
            if (smc->env.symbols.at[sym].type.basetype != TYPE_SM_STATE) {
                SMVar *var = (SMVar *)smc->env.symbols.at[sym].value;
                if (!var->has_state)
                    var->match = NULL;
            }
        }

        if (!expr_match(smc, expr, matches.at[i]->expr_pattern))
            continue;
        
        // If the match was an action.
        if (sv_is_null(matches.at[i]->next_state_true)) {
            sm_action(smc, &expr->loc, matches.at[i]->action);
            return true;
        }

        Symbol *sym_state_true
            = env_find(&smc->env, matches.at[i]->next_state_true);
        assert(sym_state_true && sym_state_true->type.basetype == TYPE_SM_STATE);

        // If there's a second state, i.e. if the SM state should split on
        // a predicate, set a variable in smc so that visit_cfg_node knows
        // to update the state of the false path differently when it
        // branches.
        if (!sv_is_null(matches.at[i]->next_state_false)) {
            Symbol *sym_state_false
                = env_find(&smc->env, matches.at[i]->next_state_false);
            assert(sym_state_false 
                && sym_state_false->type.basetype == TYPE_SM_STATE);

            smc->false_next = (StmtSMState *)sym_state_false->value;
        }

        // If the (true) state to transition to isn't a variable one, just
        // set the global state.
        StmtSMState *state_true = (StmtSMState *)sym_state_true->value;
        if (sv_is_null(state_true->var)) {
            smc->state = state_true;
            return true;
        }

        // Otherwise, transition to a variable's state. If variable
        // does not yet have a state, instantiate new SM to handle it.
        // False case is again handled in visit_cfg_node.
        Symbol *varsym = env_find(&smc->env, matches.at[i]->next_state_var);
        SMVar *var = (SMVar *)varsym->value;
        assert(var->has_state);

        // TODO: spawn new SM if variable previously did not have state
        //       in order to handle e.g. multiple allocations on a path
        // problem: if we instantiate a new sm here, it can't know where
        // to continue execution, since that information is in
        // visit_cfg_node
        // but if we instantiate a new sm in visit_cfg_node, that has
        // to finish iterating through a whole block before it does
        // any instantiation
        // if (!var->state) {
        //     SMChecker copy = smc_copy(smc, smc->arena);
        //     var->state = state_true;
        //     visit_cfg_node(&copy, copy.current_node, ++copy.stmt_in_node)
            
        // }
        var->state = state_true;
        return true;
    }

    return false;
}

static void transition(SMChecker *smc, Expr *expr) {
    smc->false_next = NULL;

    if (check_matches(smc, expr, smc->state->matches))
        return;

    for (size_t var_i = 0; var_i < smc->env.symbols.count; var_i++) {
        if (smc->env.symbols.at[var_i].type.basetype == TYPE_SM_STATE)
            continue;
        
        VEC_PTR_ExprSMMatch matches;
        {
            SMVar *v = (SMVar *)smc->env.symbols.at[var_i].value;
            if (!v->has_state) continue;
            matches = v->state->matches;
        }

        check_matches(smc, expr, matches);
    }
}

static void visit_Expr(SMChecker *smc, Expr *expr) {
    transition(smc, expr);

    switch (expr->type) {
        case NODE_ExprVar:
            return;
        case NODE_ExprArray: {
            ExprArray *e = (ExprArray *)expr;
            visit_Expr(smc, e->left);
            visit_Expr(smc, e->index);
            return;
        }
        case NODE_ExprAccess: {
            ExprAccess *e = (ExprAccess *)expr;
            visit_Expr(smc, e->left);
            return;
        }
        case NODE_ExprBinary: {
            ExprBinary *e = (ExprBinary *)expr;
            visit_Expr(smc, e->left);
            visit_Expr(smc, e->right);
            return;
        }
        case NODE_ExprUnary: {
            ExprUnary *e = (ExprUnary *)expr;
            visit_Expr(smc, e->right);
            return;
        }
        case NODE_ExprCall: {
            ExprCall *e = (ExprCall *)expr;
            for (size_t i = 0; i < e->args.count; i++)
                visit_Expr(smc, e->args.at[i]);
            return;
        }
        case NODE_ExprIntLit:
            return;
        case NODE_ExprStrLit:
            return;

        case NODE_ExprSMMatch:
        default:
            UNREACHABLE;
    }
}

static void visit_Stmt(SMChecker *smc, Stmt *stmt) {
    switch (stmt->type) {
        case NODE_StmtImport:
            UNIMPLEMENTED;
        case NODE_StmtAssign: {
            StmtAssign *st = (StmtAssign *)stmt;
            visit_Expr(smc, st->left);
            visit_Expr(smc, st->right);
            return;
        }
        case NODE_StmtVarDecl: {
            StmtVarDecl *st = (StmtVarDecl *)stmt;
            visit_Expr(smc, st->initializer);
            return;
        }
        case NODE_StmtFuncDecl: {
            StmtFuncDecl *st = (StmtFuncDecl *)stmt;
            for (size_t i = 0; i < st->body.count; i++)
                visit_Stmt(smc, st->body.at[i]);
            return;
        }
        case NODE_StmtBlock: {
            StmtBlock *st = (StmtBlock *)stmt;
            for (size_t i = 0; i < st->stmts.count; i++)
                visit_Stmt(smc, st->stmts.at[i]);
            return;
        }
        case NODE_StmtReturn:
            return;
        case NODE_StmtIf: {
            StmtIf *st = (StmtIf *)stmt;
            visit_Expr(smc, st->condition);

            // If flow-sensitive, only need to visit condition here;
            // checking the two paths will be left up to the CFG walker.
            if (!smc->sm->flow_insensitive)
                return;

            visit_Stmt(smc, st->then_branch);

            if (st->else_branch)
                visit_Stmt(smc, st->else_branch);
            
            return;
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
            visit_Expr(smc, ((StmtExpr *)stmt)->expr);
            return;

        case NODE_StmtSMDecl:
        case NODE_StmtSMState:
            return;
        default: UNREACHABLE;
    }
}

static const int TYPE_TABLE[] = {
    [TOK_ANY] = TYPE_SM_ANY,
    [TOK_ANY_ARGS] = TYPE_SM_ANY_ARGS,
    [TOK_ANY_CALL] = TYPE_SM_ANY_CALL,
};

static void visit_cfg_node(SMChecker *smc, CFGNode *node, StmtSMState* false_next) {
    smc->current_node = node;
    
    for (size_t i = 0; i < node->block.count; i++) {
        smc->stmt_in_node = i;
        smc->last = node->block.at[i]->loc;
        visit_Stmt(smc, node->block.at[i]);
    }

    if (!node->branch_a) {
        transition(smc, 
            (Expr*)&(ExprSMEndOfPath){ 
            .base.loc = smc->last,
            .base.type = NODE_ExprSMEndOfPath
        });
        return;
    }
    
    // Don't split if there's only one branch.
    if (!node->branch_b) {
        visit_cfg_node(smc, node->branch_a, NULL);
        return;
    }

    // Split the SM.
    SMChecker copy = smc_copy(smc, smc->arena);
    if (smc->false_next) {
        if (sv_is_null(false_next->var))
            copy.state = false_next;
        else {
            Symbol *varsym = env_find(&copy.env, false_next->var);
            assert(varsym);
            SMVar *var = (SMVar *)varsym->value;

            var->state = false_next;
        }
    }
    
    visit_cfg_node(smc, node->branch_a, NULL);
    visit_cfg_node(&copy, node->branch_b, NULL);
    if (copy.had_error)
        smc->had_error = true;
}

static bool run_sm(SMChecker *smc, VEC_PTR_Stmt *ast, StmtSMDecl *sm) {
    env_init(&smc->env);
    smc->sm = sm;

    bool failed = false;
    size_t i = 0;
    
    for (; i < sm->body.count
           && sm->body.at[i]->type == NODE_StmtVarDecl; i++) {
        StmtVarDecl *decl = (StmtVarDecl *)sm->body.at[i];

        SMVar *var_struct = arena_alloc(smc->arena, sizeof(SMVar));
        assert(var_struct);

        var_struct->match = NULL;
        var_struct->has_state = decl->smvar_has_state;
        var_struct->state = NULL;

        assert(decl->type.type < ARRLEN(TYPE_TABLE));
        Symbol *sym = env_insert(&smc->env, decl->name,
            TYPE_TABLE[decl->type.type], var_struct, smc->arena);

        if (!sym) {
            failed = true;
            smc_error(smc, &decl->base.loc,
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
            smc_error(smc, &state->base.loc,
                SV("Failed to insert state."));
        }
    }
    if (failed) return false;

    for (size_t stmt = 0; stmt < ast->count; stmt++) {
        if (ast->at[stmt]->type != NODE_StmtFuncDecl)
            continue;
        
        StmtFuncDecl *st = (StmtFuncDecl *)ast->at[stmt];
        
        if (sm->flow_insensitive) {
            visit_Stmt(smc, (Stmt *)st);
            if (smc->had_error)
                failed = true;
        } else {
            smc->last = st->base.loc;
            CFGNode *cfg = generate_cfg(&st->body, smc->arena);
            visit_cfg_node(smc, cfg, NULL);
            if (smc->had_error)
                failed = true;
        }
    }

    return !failed;
}

bool run_sm_checker(StringView path, Arena *arena) {
    char *source = alloc_source(path, arena);
    VEC_PTR_Stmt *ast = generate_ast(path, source, arena);
    if (!ast) return false;

    SMChecker smc;
    smc.source = source;
    smc.arena = arena;
    smc.file_path = path;
    smc.had_error = false;

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