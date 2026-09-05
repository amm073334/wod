#include <stdio.h>

#include "typechecker.h"
#include "path.h"
#include "error.h"

VEC_PTR_DEF(StmtDBTypeDecl);

typedef struct Typechecker {
    // Tracks environments for the current file.
    Environment *top_level_env;
    Environment *current_env;

    // Used to make sure that return types are right, etc.
    StmtCevDecl *current_func;

    // Used to make sure that continue/break statements are
    // only used within loops.
    size_t loop_depth;

    // The list of unqualified imports in the file currently
    // being typechecked. This list is to be searched if no
    // symbols in the current file match.
    VEC_PTR_Environment unqualified_imports;

    Arena *arena;
    VEC_Module *modules;

    // There should only be one of each of these across an entire project.
    StmtDefDB *def_udb;
    StmtDefDB *def_cdb;

    // As the global def lists of all UDB/CDB types can be defined in any file, the compiler
    // may not have encountered those lists before coming across the declaration
    // of a DB type. Therefore, it needs to keep track of those declarations so that
    // it can verify after checking all modules that every DB type declaration has a
    // corresponding entry in the def list.
    VEC_PTR_StmtDBTypeDecl all_udb_decls;
    VEC_PTR_StmtDBTypeDecl all_cdb_decls;

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
    if (a.basetype != b.basetype) return false;
    
    switch (a.basetype) {
        case TYPE_NONE:
        case TYPE_ERROR:
        case TYPE_VOID:
        case TYPE_MODULE:
            UNREACHABLE;
            return false;

        case TYPE_INT:
        case TYPE_STR:
        case TYPE_BOOL:
            return true;

        case TYPE_PTR: 
            return wt_equal(*a.ptr_to, *b.ptr_to);

        case TYPE_FUNC:
            if (wt_equal(*a.return_type, *b.return_type))
                return false;

            if (a.params.count != b.params.count)
                return false;
        
            for (size_t i = 0; i < a.params.count; i++) {
                if (!wt_equal(a.params.at[i], b.params.at[i]))
                    return false;
            }

            return true;

        case TYPE_DBDATA:
            return wt_equal(a.db_type->type, b.db_type->type);

        case TYPE_ARRAY:
            return a.array_len == b.array_len 
                && wt_equal(*a.array_of, *b.array_of);

        case TYPE_DBTYPE:
            return a.db_fields == b.db_fields;

        case TYPE_CEVTYPE:
            UNIMPLEMENTED;
            return false;
    }
}

static void tc_error(Typechecker *tc, Token *token, StringView message) {
    tc->had_error = true;
    error(token->loc, message);
}

static void tc_expr_error(Typechecker *tc, Expr *expr, Token *token, StringView message) {
    if (expr->type.basetype == TYPE_ERROR) return;
    expr->type.basetype = TYPE_ERROR;
    tc_error(tc, token, message);
}

static Symbol *find_including_imports(Typechecker *tc, Environment *env, StringView name) {
    Symbol *sym = env_find_recursive(env, name);
    if (sym) return sym;

    size_t num_found = 0;
    for (size_t i = 0; i < tc->unqualified_imports.count; i++) {
        sym = env_find(tc->unqualified_imports.at[i], name);

        // Only increment the number found if the symbol wasn't a module alias.
        // (Otherwise, it would make for weird importing semantics.)
        if (sym && sym->type.basetype != TYPE_MODULE) num_found++;
        
        // If more than one symbol with the same name is found,
        // this is ambiguous; don't return a result.
        if (num_found > 1) {
            return NULL;
        }
    }

    return sym;
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
        e->sym = find_including_imports(tc, tc->current_env, e->name);
        if (!e->sym) {
            tc_expr_error(tc, expr, &e->base.tok, SV("Undeclared or ambiguous identifier."));
        } else {
            expr->type = e->sym->type;
        }
        return;
    }
    case NODE_ExprArray: {
        ExprArray *e = (ExprArray *)expr;

        visit_Expr(tc, e->left);

        if (e->left->type.basetype != TYPE_ARRAY
            && e->left->type.basetype != TYPE_DBTYPE) {
            tc_expr_error(tc, expr, &e->left->tok,
                SV("Tried to index into non-array type."));
        }

        visit_Expr(tc, e->index);

        if (e->index->type.basetype != TYPE_INT) {
            tc_expr_error(tc, expr, &e->index->tok,
                SV("Array index must be integer type."));
        }
        
        if (e->left->type.basetype != TYPE_DBTYPE
            && !e->index->type.is_constexpr) {

            tc_expr_error(tc, expr, &e->index->tok,
                SV("Currently, indices of array accesses must be constant expressions."));
        }

        if (expr->type.basetype != TYPE_ERROR) {
            if (e->left->type.basetype == TYPE_DBTYPE) {
                expr->type = (WodType){
                    .basetype = TYPE_DBDATA,
                    .is_assignable = false,
                    .is_constexpr = false,
                    .db_type = &e->left->type
                };
            } else {
                expr->type = *e->left->type.array_of;

                if (!expr->type.is_constexpr)
                    expr->type.is_assignable = true;
            }
        }
        return;
    }
    case NODE_ExprAccess: {
        ExprAccess *e = (ExprAccess *)expr;
        visit_Expr(tc, e->left);

        Symbol *search = NULL;

        switch (e->left->type.basetype) {
        case TYPE_DBDATA: {
            search = env_find(e->left->type.db_type->type.db_fields, e->name.text);
            break;
        }

        // NOTE: This case currently can't happen, since module aliases have been
        //       disabled as a feature.
        case TYPE_MODULE:
            assert(e->left->type.module_env);
            search = env_find(e->left->type.module_env, e->name.text);
            break;

        case TYPE_DBTYPE:
            tc_expr_error(tc, expr, &e->left->tok,
                SV("Tried to access member of DB directly."));
            break;

        default:
            tc_expr_error(tc, expr, &e->left->tok,
                SV("Tried to access member of type that has no members."));
            break;
        }

        e->sym = search;

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

        if (!wt_equal(e->left->type, e->right->type))
            tc_expr_error(tc, expr, &e->left->tok,
                SV("Operands of binary operation are of different types."));

        BaseType bt;
        switch (e->op.type) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:
        case TOK_LESS_LESS:
        case TOK_GREATER_GREATER:
        case TOK_AMP:
        case TOK_PIPE:
        case TOK_CARET: {
            bt = TYPE_INT;

            StringView err = 
                SV("Operand of arithmetic operation must be of integer type.");
            if (e->left->type.basetype != TYPE_INT)    
                tc_expr_error(tc, expr, &e->left->tok, err);
            if (e->right->type.basetype != TYPE_INT)
                tc_expr_error(tc, expr, &e->right->tok, err);
            break;
        }
        case TOK_GREATER:
        case TOK_GREATER_EQUAL:
        case TOK_LESS:
        case TOK_LESS_EQUAL: {
            bt = TYPE_BOOL;

            StringView err = 
                SV("Operand of comparison operation must be of integer type.");
            if (e->left->type.basetype != TYPE_INT)    
                tc_expr_error(tc, expr, &e->left->tok, err);
            if (e->right->type.basetype != TYPE_INT)
                tc_expr_error(tc, expr, &e->right->tok, err);
            break;
        }
        case TOK_EQUAL_EQUAL:
        case TOK_BANG_EQUAL: {
            bt = TYPE_BOOL;

            StringView err = 
                SV("Operand of comparison operation must be of integer or boolean type.");
            if (e->left->type.basetype != TYPE_INT && e->left->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->left->tok, err);
            if (e->right->type.basetype != TYPE_INT && e->right->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->right->tok, err);
            break;
        }
        case TOK_AMP_AMP:
        case TOK_PIPE_PIPE: {
            bt = TYPE_BOOL;

            StringView err = 
                SV("Operand of logical operation must be of boolean type.");
            if (e->left->type.basetype != TYPE_BOOL)    
                tc_expr_error(tc, expr, &e->left->tok, err);
            if (e->right->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->right->tok, err);
            break;
        }
        default:
            bt = TYPE_ERROR;
            tc_expr_error(tc, expr, &e->right->tok, SV("Unsupported operation."));
            break;
        }

        if (expr->type.basetype != TYPE_ERROR) {
            expr->type = (WodType){
                .basetype = bt,
                .is_assignable = false,
                .is_constexpr =
                    e->left->type.is_constexpr
                    && e->right->type.is_constexpr
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
                tc_expr_error(tc, expr, &e->right->tok,
                    SV("Operand of negation operator must be integer type."));
            break;
        case TOK_BANG:
            if (e->right->type.basetype != TYPE_BOOL)
                tc_expr_error(tc, expr, &e->right->tok,
                    SV("Operand of logical NOT operator must be boolean type."));
            break;
        case TOK_AMP:
            if (e->right->kind != NODE_ExprVar)
                tc_expr_error(tc, expr, &e->right->tok,
                    SV("Operand of address-of operator must be a variable."));
            break;
        default: UNREACHABLE;
        }

        if (expr->type.basetype != TYPE_ERROR) {
            if (e->op.type == TOK_AMP) {
                expr->type = (WodType){
                    .basetype = TYPE_PTR,
                    .ptr_to = &((ExprVar *)e->right)->sym->type,
                    .is_assignable = false,
                    .is_constexpr = false
                };
            } else {
                expr->type = (WodType){
                    .basetype = e->right->type.basetype,
                    .is_assignable = false,
                    .is_constexpr = e->right->type.is_constexpr
                };
            }
        }
        return;
    }
    case NODE_ExprCall: {
        ExprCall *e = (ExprCall *)expr;

        visit_Expr(tc, e->callee);
        if (e->callee->type.basetype != TYPE_FUNC) {
            tc_expr_error(tc, expr, &e->callee->tok,
                SV("Tried to call non-callable type."));
        } else if (e->callee->type.params.count != e->args.count) {
            tc_expr_error(tc, expr, &e->args.at[e->args.count - 1]->tok,
                SV("Insufficient number of arguments to function call."));
        } else {
            for (size_t i = 0; i < e->args.count; i++) {
                visit_Expr(tc, e->args.at[i]);
                if (!wt_equal(e->callee->type.params.at[i], e->args.at[i]->type))
                    tc_expr_error(tc, expr, &e->args.at[i]->tok,
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
            .is_constexpr = true
        };
        return;
    }
    case NODE_ExprStrLit: {
        expr->type = (WodType){
            .basetype = TYPE_STR,
            .is_assignable = false,
            .is_constexpr = true
        };
        return;
    }
    case NODE_ExprBoolLit: {
        expr->type = (WodType){
            .basetype = TYPE_BOOL,
            .is_assignable = false,
            .is_constexpr = true
        };
        return;
    }
    case NODE_ExprInterp: {
        Expr *p = expr;
        while (p->kind == NODE_ExprInterp) {
            ExprInterp *e = (ExprInterp *)p;
            visit_Expr(tc, e->expr);
            p = e->next;
        }
        assert(p->kind == NODE_ExprStrLit);
        visit_Expr(tc, p);

        expr->type = (WodType){
            .basetype = TYPE_STR,
            .is_assignable = false,
            .is_constexpr = false
        };
        return;
    }
    case NODE_ExprStructLitField:
    case NODE_ExprDBDataElem:
        UNREACHABLE;
    }
}

static Symbol *try_insert_vardecl(Typechecker *tc, StmtVarDecl *s) {
    WodType ty = (WodType){
        .is_assignable = !s->is_const,
        .is_constexpr = s->is_const
    };

    switch (s->type.type) {
        case TOK_INT:  ty.basetype = TYPE_INT; break;
        case TOK_STR:  ty.basetype = TYPE_STR; break;
        case TOK_BOOL: ty.basetype = TYPE_BOOL; break;
        // TODO: other variable types
        default: UNREACHABLE;
    }

    if (s->array_length) {
        WodType *array_of = arena_alloc_assert(tc->arena, sizeof(WodType));
        *array_of = ty;
        ty.basetype = TYPE_ARRAY;
        ty.array_of = array_of;
    }
    s->sym = env_insert(tc->current_env, s->name, ty, s->base.tok, s->initializer != NULL, tc->arena);

    if (!s->sym) {
        tc_error(tc, &s->base.tok, SV("Redeclaration of name."));
        return NULL;
    }

    if (s->array_length) {
        visit_Expr(tc, s->array_length);
        if (!s->array_length->type.is_constexpr)
            tc_error(tc, &s->array_length->tok,
                SV("Array length must be a constant expression."));
    }

    if (s->is_const && !s->initializer)
        tc_error(tc, &s->base.tok,
            SV("Variable marked 'const' must have initializer."));

    if (s->initializer) {
        visit_Expr(tc, s->initializer);
        if (!wt_equal(s->sym->type, s->initializer->type))
            tc_error(tc, &s->initializer->tok,
                SV("Initializer does not match declared type of variable."));

        if (s->is_const && !s->initializer->type.is_constexpr)
            tc_error(tc, &s->initializer->tok,
                SV("Used non-constant expression to initialize 'const' variable."));
    }

    return s->sym;
}

static void check_data_element(Typechecker *tc, Symbol *db_type, ExprDBDataElem *data) {
    assert(db_type->type.basetype == TYPE_DBTYPE);
    
    // Check that all listed fields/assignments are sane.
    for (size_t j = 0; j < data->fields.count; j++) {
        ExprStructLitField *field = data->fields.at[j];
        Symbol *sym = env_find(db_type->type.db_fields, field->name);
        if (!sym)
            tc_error(tc, &field->base.tok, SV("No such field in DB type declaration."));

        visit_Expr(tc, field->value);
        if (!field->value->type.is_constexpr)
            tc_error(tc, &field->value->tok,
                SV("Value used in definition of DB data element must be constant expression."));

        if (sym) {
            if (!wt_equal(sym->type, field->value->type)) {
                tc_error(tc, &field->value->tok, SV("Assignment of mismatched types."));
            }
        }
    }

    // Any fields that have not been given a default value in the DB type declaration must be given a value.
    for (size_t j = 0; j < db_type->type.db_fields->symbols.count; j++) {
        Symbol sym = db_type->type.db_fields->symbols.at[j];
        if (sym.defined) continue;

        bool found = false;
        for (size_t k = 0; k < data->fields.count; k++) {
            ExprStructLitField *field = data->fields.at[k];
            if (sv_equals(sym.name, field->name)) {
                found = true;
                break;
            }
        }

        if (!found) {
            tc_error(tc, &data->base.tok, 
                SV("DB field without a default value was not given a value in definition of data element."));
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
            tc_error(tc, &s->left->tok, SV("Cannot assign to this expression."));

        if (s->left->type.basetype == TYPE_STR
            && s->assign_type.type != TOK_EQUAL)
            tc_error(tc, &s->assign_type,
                SV("Only simple assignment ('=') can be used for strings."));

        visit_Expr(tc, s->right);

        if (!wt_equal(s->left->type, s->right->type))
            tc_error(tc, &stmt->tok, SV("Assignment of incompatible types."));

        return;
    }
    case NODE_StmtVarDecl: {
        StmtVarDecl *s = (StmtVarDecl *)stmt;

        try_insert_vardecl(tc, s);
        return;
    }
    case NODE_StmtCevDecl: {
        StmtCevDecl *s = (StmtCevDecl *)stmt;
        
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
        
        Symbol *f = env_find_recursive(tc->top_level_env, tc->current_func->name);
        assert(f);
        
        if (s->expr && f->type.return_type->basetype == TYPE_VOID) {
            tc_error(tc, &s->expr->tok, SV("Returned a value in common event returning void."));
            return;
        }

        if (!s->expr && f->type.return_type->basetype != TYPE_VOID) {
            tc_error(tc, &s->base.tok, SV("No return value in common event returning non-void."));
            return;
        }

        if (s->expr) {
            visit_Expr(tc, s->expr);
    
            if (!wt_equal(*f->type.return_type, s->expr->type))
                tc_error(tc, &s->expr->tok, SV("Return type mismatch."));
        }

        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        visit_Expr(tc, s->condition);
        if (s->condition->type.basetype != TYPE_BOOL)
            tc_error(tc, &s->condition->tok, SV("'if' condition must be boolean type."));

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
                tc_error(tc, &s->count->tok,
                    SV("Loop count must be integer type."));
        }

        tc->loop_depth++;
        open_scope(tc);
        visit_Stmt(tc, s->body);
        close_scope(tc);
        tc->loop_depth--;
        return;
    }
    case NODE_StmtForC: {
        StmtForC *s = (StmtForC *)stmt;
        open_scope(tc);

        if (s->init)
            visit_Stmt(tc, s->init);

        if (s->condition) {
            visit_Expr(tc, s->condition);
            if (s->condition->type.basetype != TYPE_BOOL)
                tc_error(tc, &s->condition->tok,
                    SV("Condition must be of boolean type."));
        }

        if (s->iter_stmt)
            visit_Stmt(tc, s->iter_stmt);

        tc->loop_depth++;
        visit_Stmt(tc, s->body);
        tc->loop_depth--;

        close_scope(tc);
        return;
    }
    case NODE_StmtForRange: {
        StmtForRange *s = (StmtForRange *)stmt;
        open_scope(tc);

        if (s->decl->is_const)
            tc_error(tc, &s->decl->base.tok, SV("Iterator cannot be 'const'."));

        Symbol *sym = try_insert_vardecl(tc, s->decl);
        if (sym) {
            if (sym->type.basetype != TYPE_INT)
                tc_error(tc, &s->decl->type, SV("'for' variable must be integer type."));

            // Do not allow assigning to the iterator variable of a range loop.
            sym->type.is_assignable = false;
        }
        
        visit_Expr(tc, s->right_bound);
        if (s->right_bound->type.basetype != TYPE_INT)
            tc_error(tc, &s->right_bound->tok,
                SV("Iteration bound must be of integer type."));

        tc->loop_depth++;
        visit_Stmt(tc, s->body);
        tc->loop_depth--;

        close_scope(tc);
        return;
    }
    case NODE_StmtContinue: {
        if (tc->loop_depth == 0) 
            tc_error(tc, &stmt->tok,
                SV("A 'continue' statement can only be used within a loop."));
        return;
    }
    case NODE_StmtBreak: {
        if (tc->loop_depth == 0) 
            tc_error(tc, &stmt->tok,
                SV("A 'break' statement can only be used within a loop."));
        return;
    }
    case NODE_StmtCmd: {
        StmtCmd *s = (StmtCmd *)stmt;
        visit_Expr(tc, s->id);
        if (s->id->type.basetype != TYPE_INT)
            tc_error(tc, &s->id->tok, SV("Command ID must be integer type."));
        
        if (!s->id->type.is_constexpr)
            tc_error(tc, &s->id->tok, SV("Command ID must be constant expression."));

        for (size_t i = 0; i < s->int_operands.count; i++) {
            Expr *op = s->int_operands.at[i];
            visit_Expr(tc, op);
            if (op->type.basetype != TYPE_INT
                && op->type.basetype != TYPE_PTR)
                tc_error(tc, &op->tok,
                    SV("Argument must be integer or pointer type."));

            if (op->type.basetype != TYPE_PTR
                && !op->type.is_constexpr)
                tc_error(tc, &op->tok,
                    SV("Argument must be constant expression."));
        }

        for (size_t i = 0; i < s->str_operands.count; i++) {
            Expr *op = s->str_operands.at[i];
            visit_Expr(tc, op);
            if (op->type.basetype != TYPE_STR)
                tc_error(tc, &op->tok,
                    SV("Argument must be string type."));

            if (op->kind != NODE_ExprInterp &&
                !op->type.is_constexpr)
                tc_error(tc, &op->tok,
                    SV("Argument must be constant expression."));
        }
        return;
    }
    case NODE_StmtCall: {
        StmtCall *s = (StmtCall *)stmt;
        visit_Expr(tc, (Expr *)s->call);
        return;
    }
    case NODE_StmtInc: {
        StmtInc *s = (StmtInc *)stmt;
        visit_Expr(tc, s->expr);
        if (!s->expr->type.is_assignable)
            tc_error(tc, &s->base.tok, SV("Cannot increment this expression."));

        if (s->expr->type.basetype != TYPE_INT)
            tc_error(tc, &s->base.tok, SV("Can only increment integers."));
        return;
    }
    case NODE_StmtDec: {
        StmtDec *s = (StmtDec *)stmt;
        visit_Expr(tc, s->expr);
        if (!s->expr->type.is_assignable)
            tc_error(tc, &s->base.tok, SV("Cannot decrement expression."));

        if (s->expr->type.basetype != TYPE_INT)
            tc_error(tc, &s->base.tok, SV("Can only decrement integers."));
        return;
    }
    case NODE_StmtDefDB: {
        StmtDefDB *s = (StmtDefDB *)stmt;
        for (size_t i = 0; i < s->db_types.count; i++) {
            Token db_type = s->db_types.at[i];

            Symbol *sym = find_including_imports(tc, tc->current_env, db_type.text);

            if (sym
                && sym->type.basetype == TYPE_DBTYPE
                && sym->type.db_kind == s->db) {
                VEC_PUSH(s->symbols, sym, tc->arena);
                continue;
            }

            if (!sym) {
                tc_error(tc, &db_type, SV("Undeclared or ambiguous identifier."));
            } else if (sym->type.basetype != TYPE_DBTYPE) {
                tc_error(tc, &db_type, SV("List item is not a DB type."));
            } else if (sym->type.db_kind != s->db) {
                tc_error(tc, &db_type, SV("List item is not of the correct DB kind."));
            }

            VEC_PUSH(s->symbols, NULL, tc->arena);
        }

        // Check that the same DB type declaration isn't being used twice in the list.
        assert(s->db_types.count == s->symbols.count);
        for (size_t i = 0; i < s->symbols.count; i++) {
            Symbol *si = s->symbols.at[i];
            if (!si) continue;
            for (size_t j = i + 1; j < s->db_types.count; j++) {
                Symbol *sj = s->symbols.at[j];
                if (!sj) continue;
                if (si == sj)
                    tc_error(tc, &s->db_types.at[j], SV("DB type was already used in this 'def' list."));
            }
        }

        return;
    }
    case NODE_StmtDBTypeDecl: {
        StmtDBTypeDecl *s = (StmtDBTypeDecl *)stmt;
        if (!s->sym) return;

        assert(tc->current_env = tc->top_level_env);
        tc->current_env = s->sym->type.db_fields;
        for (size_t i = 0; i < s->fields.count; i++) {
            StmtVarDecl *field = s->fields.at[i];
            assert(!field->is_const);

            if (field->is_const)
                tc_error(tc, &field->base.tok, SV("DB field cannot be 'const'."));

            Symbol *field_sym = try_insert_vardecl(tc, field);
            if (field_sym) {
                field_sym->local_offset = i;
                if (field->initializer) {
                    if (!field->initializer->type.is_constexpr)
                        tc_error(tc, &field->initializer->tok,
                            SV("DB field initializer must be constant expression."));
                            
                    if (field->initializer->type.basetype == TYPE_STR)
                        tc_error(tc, &field->initializer->tok,
                            SV("DB field initializer cannot be a string."));
                }
            }
        }

        // For UDB types, data elements can have names. Keep track of named elements.
        if (s->db.type == TOK_UDBTYPE) {
            tc->current_env = s->sym->type.db_named_data;
            for (size_t i = 0; i < s->data.count; i++) {
                if (sv_is_null(s->data.at[i]->name)) continue;

                Symbol *sym = env_insert(s->sym->type.db_named_data, s->data.at[i]->name, (WodType){
                    .basetype = TYPE_DBDATA,
                    .is_assignable = false,
                    .is_constexpr = false,
                    .db_type = s->sym,
                }, s->data.at[i]->base.tok, !s->data.at[i]->no_body, tc->arena);

                if (sym) {
                    sym->local_offset = i;
                } else {
                    tc_error(tc, &s->data.at[i]->base.tok, SV("Duplicate data name."));
                }
            }
        }
        tc->current_env = tc->top_level_env;
        
        // Check that defined data elements actually match the specified DB type's fields.
        for (size_t i = 0; i < s->data.count; i++) {
            if (s->data.at[i]->no_body) continue;
            check_data_element(tc, s, s->data.at[i]);
        }

        return;
    }
    case NODE_StmtDBDataDecl: {
        StmtDBDataDecl *s = (StmtDBDataDecl *)stmt;

        // Check that specified UDB type exists.
        s->dbtype_sym = find_including_imports(tc, tc->current_env, s->db_type.text);

        if (!s->dbtype_sym) {
            tc_error(tc, &s->db_type, SV("Undeclared or ambiguous identifier."));
            return;
        }

        if (s->dbtype_sym->type.basetype != TYPE_DBTYPE) {
            tc_error(tc, &s->db_type, SV("Name does not correspond to a DB type."));
            return;
        }

        if (s->dbtype_sym->type.db_kind != DB_UDB) {
            tc_error(tc, &s->db_type, SV("Name does not correspond to a UDB type."));
            return;
        }

        // Check that data element is actually present in the UDB type.
        s->data_sym = env_find(s->dbtype_sym->type.db_named_data, s->data->name);
        if (!s->data_sym) {
            tc_error(tc, &s->data->base.tok, SV("No such data element listed in UDB."));
            return;
        }

        // Check that data element is not already defined.
        if (s->data_sym->defined) {
            tc_error(tc, &s->data->base.tok, SV("Data element is already defined elsewhere."));
            return;
        }

        s->data_sym->defined = true;

        // Check that contents are sane.
        check_data_element(tc, s->dbtype_sym, s->data);

        return;
    }
    }
}

static size_t find_module(VEC_Module *modules, StringView path) {
    for (size_t i = 0; i < modules->count; i++) {
        if (sv_equals(modules->at[i].source->path, path)) {
            return i;
        }
    }
    UNREACHABLE;
    return 0;
}

static Environment *typecheck_file(Typechecker *tc, size_t module_index) {
    ProgramAST *ast = tc->modules->at[module_index].ast;

    tc->unqualified_imports = (VEC_PTR_Environment)VEC_EMPTY;
    tc->top_level_env = env_new_assert(NULL, tc->arena);
    tc->current_env = tc->top_level_env;
    
    // Handle imports. If an import isn't already typechecked, then recursively
    // check through that file first.
    for (size_t i = 0; i < ast->imports.count; i++) {
        Import s = ast->imports.at[i];

        // Assumes that import paths have been canonicalized.
        // Assumes that there are no cyclic imports.
        size_t sub_index = find_module(tc->modules, s.path);
        Environment *sub_env = tc->modules->at[sub_index].env;
        assert(sub_env);

        // Add import's top-level environment to the list.
        // If the import's environment is already in the list, error.
        for (size_t j = 0; j < tc->unqualified_imports.count; j++) {
            if (tc->unqualified_imports.at[j] == sub_env) {
                tc_error(tc, &s.tok, SV("Duplicate import."));
                goto skip;
            }
        }
        VEC_PUSH(tc->unqualified_imports, sub_env, tc->arena);

        skip:;
    }

    // If any imports failed, just exit early to avoid errors later on
    // with a bunch of symbols not being found.
    if (tc->had_error) return NULL;

    // Do a pass to add all the top-level symbols.
    for (size_t i = 0; i < ast->stmts.count; i++) {
        Stmt *stmt = ast->stmts.at[i];
        switch (stmt->kind) {
        case NODE_StmtVarDecl: {
            StmtVarDecl *s = (StmtVarDecl *)stmt;
        
            if (!s->is_const && s->initializer) {
                tc_error(tc, &s->base.tok,
                    SV("Non-constant globals cannot have initializers."));
            }

            if (s->is_const && !s->initializer) {
                tc_error(tc, &s->base.tok,
                    SV("Constant variables must be initialized."));
            }
                    
            Symbol *sym = try_insert_vardecl(tc, s);
            if (sym) sym->top_level_path = 
                tc->modules->at[module_index].source->path;

            break;
        }
        case NODE_StmtCevDecl: {
            StmtCevDecl *s = (StmtCevDecl *)stmt;

            WodType wt = { .basetype = TYPE_FUNC,
                .is_assignable = false, .is_constexpr = false,
                .is_exaddr = s->is_exaddr };

            WodType *ret = arena_alloc_assert(tc->arena, sizeof(WodType));
            wt.return_type = ret;

            switch (s->ret.type) {
            case TOK_VOID: ret->basetype = TYPE_VOID; break;
            case TOK_INT:  ret->basetype = TYPE_INT; break;
            case TOK_STR:  ret->basetype = TYPE_STR; break;
            case TOK_BOOL: ret->basetype = TYPE_BOOL; break;
            default: UNREACHABLE;
            }

            size_t i_param_size = 0;
            size_t s_param_size = 0;
            for (size_t param_i = 0; param_i < s->params.count; param_i++) {
                // TODO: handle array params
                StmtVarDecl *param = s->params.at[param_i];
                if (param->is_const)
                    tc_error(tc, &param->base.tok, SV("Function parameter cannot be 'const'."));

                switch (param->type.type) {
                case TOK_INT:
                    i_param_size++;
                    VEC_PUSH(wt.params,
                        (WodType){ .basetype = TYPE_INT }, tc->arena);
                    break;
                case TOK_STR:
                    s_param_size++;
                    VEC_PUSH(wt.params,
                        (WodType){ .basetype = TYPE_STR }, tc->arena);
                    break;
                case TOK_BOOL:
                    i_param_size++;
                    VEC_PUSH(wt.params,
                        (WodType){ .basetype = TYPE_BOOL }, tc->arena);
                    break;
                default: UNREACHABLE;
                }

                if (i_param_size > 5)
                    tc_error(tc, &param->base.tok,
                        SV("More than 5 integer parameters are not currently supported."));
                
                if (s_param_size > 5)
                    tc_error(tc, &param->base.tok,
                        SV("More than 5 string parameters are not currently supported."));
            }

            s->sym = env_insert(tc->current_env, s->name,
                wt, stmt->tok, true, tc->arena);

            if (!s->sym)
                tc_error(tc, &s->base.tok, SV("Redeclaration of name."));
            else s->sym->top_level_path = 
                tc->modules->at[module_index].source->path;

            break;
        }
        case NODE_StmtDBTypeDecl: {
            StmtDBTypeDecl *s = (StmtDBTypeDecl *)stmt;
            Environment *db_fields = env_new_assert(NULL, tc->arena);
            Environment *db_named_data = env_new_assert(NULL, tc->arena);

            int db_kind = 0;
            switch (s->db.type) {
                case TOK_UDBTYPE: db_kind = DB_UDB; break;
                case TOK_CDBTYPE: db_kind = DB_CDB; break;
                default: UNREACHABLE;
            }

            s->sym = env_insert(tc->current_env, s->name,
                (WodType){ .basetype = TYPE_DBTYPE, .is_assignable = false,
                    .db_kind = db_kind, .db_fields = db_fields, .db_named_data = db_named_data },
                    stmt->tok, true, tc->arena);

            if (!s->sym) {
                tc_error(tc, &stmt->tok, SV("Redeclaration of name."));
                break;
            } 
            
            s->sym->top_level_path = tc->modules->at[module_index].source->path;

            if (db_kind == DB_UDB)
                VEC_PUSH(tc->all_udb_decls, s, tc->arena);
            else if (db_kind == DB_CDB)
                VEC_PUSH(tc->all_cdb_decls, s, tc->arena);
            else UNREACHABLE;

            break;
        }
        case NODE_StmtDefDB: {
            StmtDefDB *s = (StmtDefDB *)stmt;
            switch (s->db) {
                case DB_UDB: {
                    if (tc->def_udb)
                        tc_error(tc, &s->base.tok, SV("Redefinition of UDB type list."));
                    else tc->def_udb = s;
                    break;
                }
                case DB_CDB: {
                    if (tc->def_cdb)
                        tc_error(tc, &s->base.tok, SV("Redefinition of CDB type list."));
                    else tc->def_cdb = s;
                    break;
                }
                default: UNREACHABLE;
            }

            if (s->db_types.count > 100) {
                tc_error(tc, &s->base.tok, SV("Exceeded 100 DB types."));
            }
            break;
        }
        default: UNREACHABLE;
        }
    }

    if (module_index == tc->modules->count - 1 && !env_find(tc->top_level_env, SV("main"))) {
        tc->had_error = true;
        source_error(*tc->modules->at[module_index].source, SV("No 'main' function."));
    }

    // Do a second pass to look inside functions.
    for (size_t i = 0; i < ast->stmts.count; i++) {
        Stmt *stmt = ast->stmts.at[i];
        if (stmt->kind == NODE_StmtCevDecl || stmt->kind == NODE_StmtDefDB)
            visit_Stmt(tc, stmt);
    }

    // Update module info.
    tc->modules->at[module_index].env = tc->top_level_env;

    return tc->had_error ? NULL : tc->top_level_env;
}

static void check_def_lists(Typechecker *tc, VEC_PTR_StmtDBTypeDecl *all_decls, StmtDefDB *def) {
    // If there is at least one DB type decl, there should be a def list.
    if (all_decls->count > 0 && !def) {
        tc->had_error = true;
        source_error(*tc->modules->at[tc->modules->count - 1].source,
            SV("Could not find DB 'def' list, but DB type declarations were found."));
        return;
    }

    // Check that all declared DB types are present in the def list.
    for (size_t i = 0; i < all_decls->count; i++) {
        bool found = false;
        for (size_t j = 0; j < def->symbols.count; j++) {
            if (all_decls->at[i]->sym == def->symbols.at[j]) {
                found = true;
                break;
            }
        }

        if (!found) {
            tc_error(&tc, &all_decls->at[i]->base.tok,
                SV("DB type was not found in the corresponding DB 'def' list."));
        }
    }
}

bool typecheck_modules(VEC_Module *modules, Arena *arena) {
    assert(modules->count > 0);

    Typechecker tc = {
        .arena = arena,
        .had_error = false,
        .loop_depth = 0,
        .current_func = NULL,
        .def_udb = NULL,
        .def_cdb = NULL,
        .all_udb_decls = VEC_EMPTY,
        .all_cdb_decls = VEC_EMPTY,
        .modules = modules,
    };
    
    for (size_t i = 0; i < modules->count; i++) {
        typecheck_file(&tc, i);
    }

    check_def_lists(&tc, &tc.all_udb_decls, tc.def_udb);
    check_def_lists(&tc, &tc.all_cdb_decls, tc.def_cdb);

    // For UDB types, data elements can be listed just by name without a body, but their body must
    // be completed elsewhere if so.
    // Check that data elements without a body are defined elsewhere.
    for (size_t i = 0; i < tc.all_udb_decls.count; i++) {
        Environment *named = tc.all_udb_decls.at[i]->sym->type.db_named_data;
        for (size_t j = 0; j < named->symbols.count; j++) {
            Symbol sym = named->symbols.at[j];
            assert(sym.type.basetype == TYPE_DBDATA);
            if (!sym.defined) {
                tc_error(&tc, &sym.declaration, SV("Data element was declared but not defined."));
            }
        }
    }

    return !tc.had_error;
}
