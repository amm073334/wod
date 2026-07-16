#include <stdio.h>

#include "typechecker.h"
#include "path.h"
#include "error.h"

typedef struct {
    // Used to avoid analyzing a file more than once.
    Environment *global_module_list;

    // Tracks environments for the current file.
    Environment *top_level_env;
    Environment *current_env;

    // Whether or not typechecking has finished the pass for top-level symbols.
    bool finished_top_level_pass;

    // Used to make sure that return types are right, etc.
    StmtFuncDecl *current_func;

    // Used to make sure that continue/break statements are
    // only used within loops.
    size_t loop_depth;

    Arena *arena;
    StringView file_path;
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

static Environment *env_new_assert(Environment *parent, Arena *arena) {
    Environment *env = env_new(parent, arena);
    if (!env) {
        fprintf(stderr, "Fatal: Out of memory.\n");
        exit(1);
    }
    return env;
}

static bool wt_equal(WodType a, WodType b) {
    // TODO: handle other types
    return a.basetype == b.basetype;
}

static void tc_error(Typechecker *tc, Token *token, StringView message) {
    tc->had_error = true;
    error(tc->file_path, tc->source, token, message);
}

static void tc_expr_error(Typechecker *tc, Expr *expr, Token *token, StringView message) {
    if (expr->type.basetype == TYPE_ERROR) return;
    expr->type.basetype = TYPE_ERROR;
    tc_error(tc, token, message);
}

static void visit_Expr(Typechecker *tc, Expr *expr) {
    expr->env = tc->current_env;

    // Initialize node's type to TYPE_NONE. If an error occurs somewhere,
    // the type gets set to TYPE_ERROR; this makes it possible to check
    // whether or not an error has occurred before doing something.
    expr->type.basetype = TYPE_NONE;

    switch (expr->kind) {
    case NODE_ExprVar: {
        ExprVar *e = (ExprVar *)expr;
        Symbol *sym = env_find(tc->current_env, e->name);
        if (!sym) {
            tc_expr_error(tc, expr, &e->base.loc, SV("Undeclared identifier."));
        } else {
            expr->type = sym->type;
        }
        return;
    }
    case NODE_ExprArray: {
        ExprArray *e = (ExprArray *)expr;

        visit_Expr(tc, e->left);

        if (e->left->type.basetype != TYPE_ARRAY
            && e->left->type.basetype != TYPE_DBTYPE) {
            tc_expr_error(tc, expr, &e->left->loc,
                SV("Tried to index into non-array type."));
        }

        visit_Expr(tc, e->index);

        if (e->index->type.basetype != TYPE_INT) {
            tc_expr_error(tc, expr, &e->index->loc,
                SV("Array index must be integer type."));
        }
        
        if (e->left->type.basetype != TYPE_DBTYPE
            && !e->index->type.is_compile_time) {

            tc_expr_error(tc, expr, &e->index->loc,
                SV("Currently, indices of array accesses must be constant expressions."));
        }

        if (expr->type.basetype != TYPE_ERROR) {
            if (e->left->type.basetype == TYPE_DBTYPE) {
                expr->type = (WodType){
                    .basetype = TYPE_DBDATA,
                    .is_assignable = false,
                    .is_compile_time = false
                };
            } else {
                expr->type = *e->left->type.array_of;

                if (!expr->type.is_compile_time)
                    expr->type.is_assignable = true;
            }
        }
        return;
    }
    case NODE_ExprAccess: {
        ExprAccess *e = (ExprAccess *)expr;
        
        Symbol *search = NULL;

        switch (e->left->type.basetype) {
        case TYPE_DBDATA: {
            Symbol *sym = env_find(tc->current_env, e->left->type.db_name);

            // If we have successfully resolved an expression to be of type
            // TYPE_DBDATA, then a check that the DB name was valid should have
            // already been done.
            assert(sym && sym->type.basetype == TYPE_DBTYPE);
            
            search = env_find(sym->type.db_env, e->name.text);
            break;
        }

        case TYPE_MODULE:
            assert(e->left->type.module_env);
            search = env_find(e->left->type.module_env, e->name.text);
            break;

        case TYPE_DBTYPE:
            tc_expr_error(tc, expr, &e->left->loc,
                SV("Tried to access member of DB directly."));
            break;

        default:
            tc_expr_error(tc, expr, &e->left->loc,
                SV("Tried to access member of type that has no members."));
            break;
        }

        if (search)
            expr->type = search->type;
        else
            tc_expr_error(tc, expr, &e->name, SV("No such member found."));

        return;
    }
    case NODE_ExprBinary: {
        ExprBinary *e = (ExprBinary *)expr;

        visit_Expr(tc, e->left);
        visit_Expr(tc, e->right);

        if (e->left->type.basetype != e->left->type.basetype)
            tc_expr_error(tc, expr, &e->left->loc,
                SV("Operands of binary operation are of different types."));

        BaseType bt;
        switch (e->op.type) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:
        case TOK_GREATER:
        case TOK_GREATER_EQUAL:
        case TOK_LESS:
        case TOK_LESS_EQUAL:
        case TOK_LESS_LESS:
        case TOK_GREATER_GREATER:
        case TOK_AMP:
        case TOK_PIPE:
        case TOK_CARET: {
            bt = TYPE_INT;

            StringView err = 
                SV("Operand of arithmetic or comparison operation must be of integer type.");
            if (e->left->type.basetype != TYPE_INT)    
                tc_expr_error(tc, expr, &e->left->loc, err);
            if (e->right->type.basetype != TYPE_INT)
                tc_expr_error(tc, expr, &e->right->loc, err);
            break;
        }
        case TOK_EQUAL_EQUAL:
        case TOK_BANG_EQUAL: {
            bt = TYPE_BOOL;

            StringView err = 
                SV("Operand of comparison operation must be of integer or boolean type.");
            if (e->left->type.basetype != TYPE_INT && e->left->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->left->loc, err);
            if (e->right->type.basetype != TYPE_INT && e->right->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->right->loc, err);
            break;
        }
        case TOK_AMP_AMP:
        case TOK_PIPE_PIPE: {
            bt = TYPE_BOOL;

            StringView err = 
                SV("Operand of logical operation must be of boolean type.");
            if (e->left->type.basetype != TYPE_BOOL)    
                tc_expr_error(tc, expr, &e->left->loc, err);
            if (e->right->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->right->loc, err);
            break;
        }
        default:
            bt = TYPE_ERROR;
            tc_expr_error(tc, expr, &e->right->loc, SV("Unsupported operation."));
            break;
        }

        if (expr->type.basetype != TYPE_ERROR) {
            expr->type = (WodType){
                .basetype = bt,
                .is_assignable = false,
                .is_compile_time =
                    e->left->type.is_compile_time
                    && e->right->type.is_compile_time
            };
        }

        return;
    }
    case NODE_ExprUnary: {
        ExprUnary *e = (ExprUnary *)expr;

        visit_Expr(tc, e->right);
        switch (e->op.type) {
        case TOK_MINUS:
            if (e->right->type.basetype != TYPE_INT)
                tc_expr_error(tc, expr, &e->right->loc,
                    SV("Operand of negation operator must be integer type."));
            break;
        case TOK_BANG:
            if (e->right->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->right->loc,
                    SV("Operand of logical NOT operator must be boolean type."));
            break;
        default: UNREACHABLE;
        }

        if (expr->type.basetype != TYPE_ERROR) {
            expr->type = (WodType){
                .basetype = e->right->type.basetype,
                .is_assignable = false,
                .is_compile_time = e->right->type.is_compile_time
            };
        }
        return;
    }
    case NODE_ExprCall: {
        ExprCall *e = (ExprCall *)expr;

        visit_Expr(tc, e->callee);
        if (e->callee->type.basetype != TYPE_FUNC) {
            tc_expr_error(tc, expr, &e->callee->loc,
                SV("Tried to call non-callable type."));
        } else if (e->callee->type.params.count != e->args.count) {
            tc_expr_error(tc, expr, &e->args.at[e->args.count - 1]->loc,
                SV("Insufficient number of arguments to function call."));
        } else {
            for (size_t i = 0; i < e->args.count; i++) {
                visit_Expr(tc, e->args.at[i]);
                if (!wt_equal(e->callee->type.params.at[i], e->args.at[i]->type))
                    tc_expr_error(tc, expr, &e->args.at[i]->loc,
                        SV("Type mismatch in call."));
            }
        }

        if (expr->type.basetype != TYPE_ERROR) {
            expr->type = *e->callee->type.return_type;
        }

        return;
    }
    case NODE_ExprIntLit: {
        expr->type = (WodType){
            .basetype = TYPE_INT,
            .is_assignable = false,
            .is_compile_time = true
        };
        return;
    }
    case NODE_ExprStrLit: {
        expr->type = (WodType){
            .basetype = TYPE_STR,
            .is_assignable = false,
            .is_compile_time = true
        };
        return;
    }
    case NODE_ExprBoolLit: {
        expr->type = (WodType){
            .basetype = TYPE_BOOL,
            .is_assignable = false,
            .is_compile_time = true
        };
        return;
    }
    }
}

static void visit_Stmt(Typechecker *tc, Stmt *stmt) {
    stmt->env = tc->current_env;
    
    switch (stmt->kind) {
    case NODE_StmtAssign: {
        StmtAssign *s = (StmtAssign *)stmt;

        visit_Expr(tc, s->left);

        if (!s->left->type.is_assignable)
            tc_error(tc, &s->left->loc, SV("Cannot assign to this expression."));

        visit_Expr(tc, s->right);

        if (!wt_equal(s->left->type, s->right->type))
            tc_error(tc, &stmt->loc, SV("Assignment of incompatible types."));

        return;
    }
    case NODE_StmtVarDecl: {
        StmtVarDecl *s = (StmtVarDecl *)stmt;

        if (s->array_length) {
            visit_Expr(tc, s->array_length);
            if (!s->array_length->type.is_compile_time)
                tc_error(tc, &s->array_length->loc,
                    SV("Array length must be a constant expression."));
        }

        WodType ty = (WodType){
            .is_assignable = !s->is_const,
            .is_compile_time = s->is_const
        };

        switch (s->type.type) {
            case TOK_INT:  ty.basetype = TYPE_INT; break;
            case TOK_STR:  ty.basetype = TYPE_STR; break;
            case TOK_BOOL: ty.basetype = TYPE_BOOL; break;
            // TODO: other variable types
            default: UNREACHABLE;
        }

        if (s->is_const && !s->initializer)
            tc_error(tc, &s->base.loc,
                SV("Variable marked 'const' must have initializer."));

        if (s->initializer) {
            visit_Expr(tc, s->initializer);
            if (ty.basetype != s->initializer->type.basetype)
                tc_error(tc, &s->initializer->loc,
                    SV("Initializer does not match declared type of variable."));

            if (s->is_const && !s->initializer->type.is_compile_time)
                tc_error(tc, &s->initializer->loc,
                    SV("Used non-constant expression to initialize 'const' variable."));
        }

        Symbol *sym = NULL;
        if (s->array_length) {
            WodType *array_of = arena_alloc_assert(tc->arena, sizeof(WodType));
            *array_of = ty;
            ty.basetype = TYPE_ARRAY;
            ty.array_of = array_of;
        }
        sym = env_insert(tc->current_env, s->name, ty, tc->arena);

        if (!sym)
            tc_error(tc, &s->base.loc, SV("Redeclaration of name."));
        
        return;
    }
    case NODE_StmtFuncDecl: {
        StmtFuncDecl *s = (StmtFuncDecl *)stmt;
        
        open_scope(tc);

        tc->current_func = s;

        for (size_t i = 0; i < s->params.count; i++)
            visit_Stmt(tc, (Stmt *)s->params.at[i]);
        for (size_t i = 0; i < s->body.count; i++)
            visit_Stmt(tc, s->body.at[i]);

        tc->current_func = NULL;

        close_scope(tc);
        return;
    }
    case NODE_StmtBlock: {
        StmtBlock *s = (StmtBlock *)stmt;
        open_scope(tc);
        for (size_t i = 0; i < s->stmts.count; i++)
            visit_Stmt(tc, s->stmts.at[i]);
        close_scope(tc);
        return;
    }
    case NODE_StmtReturn: {
        StmtReturn *s = (StmtReturn *)stmt;
        
        Symbol *f = env_find(tc->top_level_env, tc->current_func->name);
        assert(f);
        
        if (s->expr && f->type.return_type->basetype == TYPE_VOID) {
            tc_error(tc, &s->expr->loc, SV("Returned a value in common event returning void."));
            return;
        }

        if (!s->expr && f->type.return_type->basetype != TYPE_VOID) {
            tc_error(tc, &s->base.loc, SV("No return value in common event returning non-void."));
            return;
        }

        if (s->expr) {
            visit_Expr(tc, s->expr);
    
            if (!wt_equal(*f->type.return_type, s->expr->type))
                tc_error(tc, &s->expr->loc, SV("Return type mismatch."));
        }

        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        visit_Expr(tc, s->condition);
        if (s->condition->type.basetype != TYPE_BOOL)
            tc_error(tc, &s->condition->loc, SV("'if' condition must be boolean type."));

        open_scope(tc);
        visit_Stmt(tc, s->then_branch);
        close_scope(tc);

        if (s->else_branch) {
            open_scope(tc);
            visit_Stmt(tc, s->else_branch);
            close_scope(tc);
        }
        return;
    }
    case NODE_StmtLoop: {
        StmtLoop *s = (StmtLoop *)stmt;
        if (s->count) {
            visit_Expr(tc, s->count);
            if (s->count->type.basetype != TYPE_INT)
                tc_error(tc, &s->count->loc,
                    SV("Loop count must be integer type."));
        }

        tc->loop_depth++;
        open_scope(tc);
        visit_Stmt(tc, s->body);
        close_scope(tc);
        tc->loop_depth--;
        return;
    }
    case NODE_StmtFor: {
        StmtFor *s = (StmtFor *)stmt;
        open_scope(tc);

        Symbol *sym = env_insert(tc->current_env, s->iterator,
            (WodType){ .basetype = TYPE_INT, .is_assignable = false,
                .is_compile_time = false }, tc->arena);
        
        if (!sym)
            tc_error(tc, &s->base.loc, SV("Redeclaration of iterator name."));

        visit_Expr(tc, s->left_bound);
        if (s->left_bound->type.basetype != TYPE_INT)
            tc_error(tc, &s->left_bound->loc,
                SV("Iteration bound must be of integer type."));
        
        visit_Expr(tc, s->right_bound);
        if (s->right_bound->type.basetype != TYPE_INT)
            tc_error(tc, &s->right_bound->loc,
                SV("Iteration bound must be of integer type."));

        tc->loop_depth++;
        visit_Stmt(tc, s->body);
        tc->loop_depth--;

        close_scope(tc);
        return;
    }
    case NODE_StmtContinue: {
        if (tc->loop_depth == 0) 
            tc_error(tc, &stmt->loc,
                SV("A 'continue' statement can only be used within a loop."));
        return;
    }
    case NODE_StmtBreak: {
        if (tc->loop_depth == 0) 
            tc_error(tc, &stmt->loc,
                SV("A 'break' statement can only be used within a loop."));
        return;
    }
    case NODE_StmtCmd: {
        StmtCmd *s = (StmtCmd *)stmt;
        visit_Expr(tc, s->id);
        if (s->id->type.basetype != TYPE_INT)
            tc_error(tc, &s->id->loc, SV("Command ID must be integer type."));
        
        if (!s->id->type.is_compile_time)
            tc_error(tc, &s->id->loc, SV("Command ID must be constant expression."));

        for (size_t i = 0; i < s->int_operands.count; i++) {
            if (s->int_operands.at[i]->type.basetype != TYPE_INT
                && s->int_operands.at[i]->type.basetype != TYPE_PTR)
                tc_error(tc, &s->int_operands.at[i]->loc,
                    SV("Argument must be integer or pointer type."));

            if (!s->int_operands.at[i]->type.is_compile_time)
                tc_error(tc, &s->int_operands.at[i]->loc,
                    SV("Argument must be constant expression."));
        }

        for (size_t i = 0; i < s->str_operands.count; i++) {
            if (s->str_operands.at[i]->type.basetype != TYPE_STR)
                tc_error(tc, &s->str_operands.at[i]->loc,
                    SV("Argument must be string type."));

            if (!s->str_operands.at[i]->type.is_compile_time)
                tc_error(tc, &s->str_operands.at[i]->loc,
                    SV("Argument must be constant expression."));
        }
        return;
    }
    case NODE_StmtDBDecl: {
        StmtDBDecl *s = (StmtDBDecl *)stmt;
        Environment *db_env = env_new_assert(NULL, tc->arena);

        int db_kind = 0;
        switch (s->db.type) {
            case TOK_UDB: db_kind = DB_UDB; break;
            case TOK_CDB: db_kind = DB_CDB; break;
            default: UNREACHABLE;
        }

        Symbol *sym = env_insert(tc->current_env, s->name,
            (WodType){ .basetype = TYPE_DBTYPE, .is_assignable = false,
                .db_kind = db_kind, .db_env = db_env }, tc->arena);

        if (!sym) {
            tc_error(tc, &stmt->loc, SV("Redeclaration of name."));
            return;
        }

        assert(tc->current_env = tc->top_level_env);
        sym->path = tc->file_path;
        tc->current_env = db_env;
        for (size_t i = 0; i < s->fields.count; i++) {
            if (s->fields.at[i]->is_const)
                tc_error(tc, &s->fields.at[i]->base.loc,
                    SV("Cannot mark DB field as 'const'."));
            else {
                visit_Stmt(tc, (Stmt *)s->fields.at[i]);
                if (s->fields.at[i]->initializer
                    && !s->fields.at[i]->initializer->type.is_compile_time)
                    tc_error(tc, &s->fields.at[i]->initializer->loc,
                        SV("DB field initializer must be constant expression."));
            }
        }
        tc->current_env = tc->top_level_env;

        return;
    }
    case NODE_StmtExpr: {
        StmtExpr *s = (StmtExpr *)stmt;
        visit_Expr(tc, s->expr);
        return;
    }
    }
}

static Environment *typecheck_file(Typechecker *tc, ProgramAST *ast, const char *source, bool is_main_file, Arena *arena) {
    // ProgramAST *ast = generate_ast(path, source, arena);
    // if (!ast) return NULL;
    
    // TODO: try to make typechecking per-module, and handle recursive import stuff
    //       elsewhere
    Environment imports;
    env_init(&imports);
    
    {
        // // Handle imports. If an import isn't in the global table, then recursively
        // // check through that file first.
        // for (size_t i = 0; i < ast->imports.count; i++) {
        //     Import s = ast->imports.at[i];
        //     StringView module_path = get_full_path(s.path, arena);
    
        //     // First, insert into the file's environment with a NULL environment.
        //     Symbol *local_sym = env_insert(&imports, module_path,
        //         (WodType){ .basetype = TYPE_MODULE,
        //             .is_assignable = false, .module_env = NULL }, arena);
    
        //     if (!local_sym) {
        //         tc_error(tc, &s.loc, SV("Duplicate import."));
        //         continue;
        //     }
    
        //     // Insert into the global list of modules. Only handle each file once.
        //     // Then go back and update the environments to be the module's
        //     // (non-null) environment.
        //     Symbol *global_sym = env_insert(tc->global_module_list, module_path,
        //         (WodType){ .basetype = TYPE_MODULE,
        //             .is_assignable = false, .module_env = NULL }, arena);
    
        //     if (global_sym) {
        //         // TODO: this source is wrong
        //         Environment *sub_env =
        //             typecheck_file(tc, module_path, source, false, arena);
    
        //         if (!sub_env) tc->had_error = true;
    
        //         global_sym->type.module_env = sub_env;
        //         local_sym->type.module_env = sub_env;
        //     } else {
        //         global_sym = env_find(tc->global_module_list, module_path);
        //         local_sym->type.module_env = global_sym->type.module_env;
        //     }
    
        //     if (!tc->had_error)
        //         assert(local_sym->type.module_env && global_sym->type.module_env);
        // }
    }

    // If any imports failed, just exit early to avoid errors later on
    // with a bunch of symbols not being found.
    if (tc->had_error) return NULL;

    tc->source = source;

    // Do a pass to add all the top-level symbols.
    tc->top_level_env = env_new_assert(NULL, arena);
    tc->current_env = tc->top_level_env;

    for (size_t i = 0; i < ast->stmts.count; i++) {
        Stmt *stmt = ast->stmts.at[i];
        switch (stmt->kind) {
        case NODE_StmtVarDecl: {
            StmtVarDecl *s = (StmtVarDecl *)stmt;
        
            if (!s->is_const && s->initializer)
                tc_error(tc, &s->base.loc,
                    SV("Non-constant globals cannot have initializers."));

            if (s->is_const && !s->initializer)
                tc_error(tc, &s->base.loc,
                    SV("Constant variables must be initialized."));
        
            visit_Stmt(tc, stmt);

            // TODO: This doesn't work if the insert failed, since it
            //       might get the symbol for a different declaration.
            Symbol *sym = env_find(tc->current_env, s->name);
            if (sym) sym->path = tc->file_path;

            break;
        }
        case NODE_StmtFuncDecl: {
            StmtFuncDecl *s = (StmtFuncDecl *)stmt;

            WodType wt = { .basetype = TYPE_FUNC,
                .is_assignable = false, .is_compile_time = s->is_inline };

            WodType *ret = arena_alloc_assert(tc->arena, sizeof(WodType));
            wt.return_type = ret;

            switch (s->ret.type) {
            case TOK_VOID: ret->basetype = TYPE_VOID; break;
            case TOK_INT:  ret->basetype = TYPE_INT; break;
            case TOK_STR:  ret->basetype = TYPE_STR; break;
            case TOK_BOOL: ret->basetype = TYPE_BOOL; break;
            default: UNREACHABLE;
            }

            for (size_t param = 0; param < s->params.count; param++) {
                // TODO: handle array params
                switch (s->params.at[param]->type.type) {
                case TOK_INT:
                    VEC_PUSH(wt.params,
                        (WodType){ .basetype = TYPE_INT }, tc->arena);
                    break;
                case TOK_STR:
                    VEC_PUSH(wt.params,
                        (WodType){ .basetype = TYPE_STR }, tc->arena);
                    break;
                case TOK_BOOL:
                    VEC_PUSH(wt.params,
                        (WodType){ .basetype = TYPE_BOOL }, tc->arena);
                    break;
                default: UNREACHABLE;
                }
            }

            Symbol *sym = env_insert(tc->current_env, s->name,
                wt, tc->arena);

            if (!sym)
                tc_error(tc, &s->base.loc, SV("Redeclaration of name."));
            else sym->path = tc->file_path;

            break;
        }
        case NODE_StmtDBDecl:
            visit_Stmt(tc, stmt);
            break;
        default: UNREACHABLE;
        }
    }

    if (is_main_file && !env_find(tc->top_level_env, SV("main")))
        tc_error(tc, NULL, SV("No 'main' function."));

    tc->finished_top_level_pass = true;

    // Do a second pass to look inside functions.
    for (size_t i = 0; i < ast->stmts.count; i++) {
        Stmt *stmt = ast->stmts.at[i];
        if (stmt->kind == NODE_StmtFuncDecl)
            visit_Stmt(tc, stmt);
    }

    return tc->had_error ? NULL : tc->top_level_env;
}

Environment *typecheck(ProgramAST *ast, const char *source, Arena *arena) {
    Typechecker tc;
    tc.arena = arena;
    tc.had_error = false;
    tc.finished_top_level_pass = false;
    tc.loop_depth = 0;
    tc.current_func = NULL;
    tc.file_path = ast->file;

    tc.global_module_list = arena_alloc_assert(arena, sizeof(Environment));
    env_init(tc.global_module_list);

    // return typecheck_file(&tc, get_full_path(path, arena), source, true, arena);
    return typecheck_file(&tc, ast, source, true, arena);
}