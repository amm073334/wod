#include "constexpr.h"

static ExprIntLit *make_int(Arena *arena, Token tok, int32_t value) {
    ExprIntLit *out = arena_alloc_assert(arena, sizeof(ExprIntLit));
    out->base.loc = tok;
    out->base.kind = NODE_ExprIntLit;
    out->base.type = (WodType){
        .basetype = TYPE_INT,
        .is_assignable = false,
        .is_compile_time = true
    };
    out->value = value;
    return out;
}

static ExprStrLit *make_str(Arena *arena, Token tok, StringView value) {
    ExprStrLit *out = arena_alloc_assert(arena, sizeof(ExprStrLit));
    out->base.loc = tok;
    out->base.kind = NODE_ExprStrLit;
    out->base.type = (WodType){
        .basetype = TYPE_STR,
        .is_assignable = false,
        .is_compile_time = true
    };
    out->value = value;
    return out;
}

static ExprBoolLit *make_bool(Arena *arena, Token tok, bool value) {
    ExprBoolLit *out = arena_alloc_assert(arena, sizeof(ExprBoolLit));
    out->base.loc = tok;
    out->base.kind = NODE_ExprBoolLit;
    out->base.type = (WodType){
        .basetype = TYPE_BOOL,
        .is_assignable = false,
        .is_compile_time = true
    };
    out->value = value;
    return out;
}

#define MAKE_INT(value) make_int(arena, expr->loc, (value))
#define MAKE_STR(value) make_str(arena, expr->loc, (value))
#define MAKE_BOOL(value) make_bool(arena, expr->loc, (value))

static void *visit_Expr(Arena *arena, Expr *expr) {
    switch (expr->kind) {
    case NODE_ExprVar: {
        ExprVar *e = (ExprVar *)expr;
        if (expr->type.is_compile_time) {
            Symbol* sym = env_find(expr->env, e->name);
            assert(sym);

            switch (expr->type.basetype) {
            case TYPE_INT: return MAKE_INT(sym->const_i);
            case TYPE_STR: return MAKE_STR(sym->const_s);
            case TYPE_BOOL: return MAKE_BOOL(sym->const_b);
            default: UNREACHABLE;
            }
        }
        return expr;
    }
    case NODE_ExprArray: {
        ExprArray *e = (ExprArray *)expr;
        e->index = visit_Expr(arena, e->index);
        e->left = visit_Expr(arena, e->left);
        return expr;
    }
    case NODE_ExprAccess: {
        ExprAccess *e = (ExprAccess *)expr;
        e->left = visit_Expr(arena, e->left);
        return expr;
    }
    case NODE_ExprBinary: {
        ExprBinary *e = (ExprBinary *)expr;
        e->left = visit_Expr(arena, e->left);
        e->right = visit_Expr(arena, e->right);

        if (!e->left->type.is_compile_time
            || !e->right->type.is_compile_time)
            return expr;

        if (e->left->kind == NODE_ExprIntLit) {
            assert(e->right->kind == NODE_ExprIntLit);
            int32_t left = ((ExprIntLit *)e->left)->value;
            int32_t right = ((ExprIntLit *)e->right)->value;

            switch (e->op.type) {
            case TOK_PLUS:            return MAKE_INT(left + right);
            case TOK_MINUS:           return MAKE_INT(left - right);
            case TOK_STAR:            return MAKE_INT(left * right);
            case TOK_SLASH:           return MAKE_INT(left / right);
            case TOK_PERCENT:         return MAKE_INT(left % right);
            case TOK_LESS_LESS:       return MAKE_INT(left << right);
            case TOK_GREATER_GREATER: return MAKE_INT(left >> right);

            case TOK_EQUAL_EQUAL:     return MAKE_BOOL(left == right);
            case TOK_BANG_EQUAL:      return MAKE_BOOL(left != right);
            case TOK_LESS:            return MAKE_BOOL(left < right);
            case TOK_LESS_EQUAL:      return MAKE_BOOL(left <= right);
            case TOK_GREATER:         return MAKE_BOOL(left > right);
            case TOK_GREATER_EQUAL:   return MAKE_BOOL(left >= right);
            default: UNREACHABLE;
            }
        } else if (e->left->kind == NODE_ExprBoolLit) {
            assert(e->right->kind == NODE_ExprBoolLit);
            bool left = ((ExprBoolLit *)e->left)->value;
            bool right = ((ExprBoolLit *)e->right)->value;
            switch (e->op.type) {
            case TOK_AMP_AMP:   return MAKE_BOOL(left && right);
            case TOK_PIPE_PIPE: return MAKE_BOOL(left || right);
            default: UNREACHABLE;
            }
        } else UNREACHABLE;
    }
    case NODE_ExprUnary: {
        ExprUnary *e = (ExprUnary *)expr;
        e->right = visit_Expr(arena, e->right);
        
        if (!e->right->type.is_compile_time)
            return expr;

        if (e->right->kind == NODE_ExprIntLit) {
            int32_t right = ((ExprIntLit *)e->right)->value;
            switch(e->op.type) {
            case TOK_MINUS: return MAKE_BOOL(-right);
            default: UNREACHABLE;
            }
        } else if (e->right->kind == NODE_ExprBoolLit) {
            bool right = ((ExprBoolLit *)e->right)->value;
            switch(e->op.type) {
            case TOK_BANG: return MAKE_BOOL(!right);
            default: UNREACHABLE;
            }
        } else UNREACHABLE;
    }
    case NODE_ExprCall: {
        ExprCall *e = (ExprCall *)expr;
        e->callee = visit_Expr(arena, e->callee);
        for (size_t i = 0; i < e->args.count; i++)
            e->args.at[i] = visit_Expr(arena, e->args.at[i]);
        return expr;
    }

    case NODE_ExprIntLit:
    case NODE_ExprStrLit:
    case NODE_ExprBoolLit:
        assert(expr->type.is_compile_time);
        return expr;
    default: UNREACHABLE;
    }

    return NULL;
}

#undef MAKE_INT
#undef MAKE_BOOL

static void visit_Stmt(Arena *arena, Stmt *stmt) {
    switch (stmt->kind) {
    case NODE_StmtAssign: {
        StmtAssign *s = (StmtAssign *)stmt;
        visit_Expr(arena, s->left);
        visit_Expr(arena, s->right);
        return;
    }
    case NODE_StmtVarDecl: {
        StmtVarDecl *s = (StmtVarDecl *)stmt;
        visit_Expr(arena, s->array_length);
        if (s->initializer)
            visit_Expr(arena, s->initializer);
        return;
    }
    case NODE_StmtFuncDecl: {
        StmtFuncDecl *s = (StmtFuncDecl *)stmt;
        for (size_t i = 0; i < s->body.count; i++)
            visit_Stmt(arena, s->body.at[i]);
        return;
    }
    case NODE_StmtBlock: {
        StmtBlock *s = (StmtBlock *)stmt;
        for (size_t i = 0; i < s->stmts.count; i++)
            visit_Stmt(arena, s->stmts.at[i]);
        return;
    }
    case NODE_StmtReturn: {
        StmtReturn *s = (StmtReturn *)stmt;
        visit_Expr(arena, s->expr);
        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        visit_Expr(arena, s->condition);
        visit_Stmt(arena, s->then_branch);
        visit_Stmt(arena, s->else_branch);
        return;
    }
    case NODE_StmtLoop: {
        StmtLoop *s = (StmtLoop *)stmt;
        visit_Expr(arena, s->count);
        visit_Stmt(arena, s->body);
        return;
    }
    case NODE_StmtFor: {
        StmtFor *s = (StmtFor *)stmt;
        visit_Expr(arena, s->left_bound);
        visit_Expr(arena, s->right_bound);
        visit_Stmt(arena, s->body);
        return;
    }
    case NODE_StmtContinue: 
    case NODE_StmtBreak: 
        return;
    case NODE_StmtCmd: {
        StmtCmd *s = (StmtCmd *)stmt;
        for (size_t i = 0; i < s->int_operands.count; i++)
            visit_Expr(arena, s->int_operands.at[i]);
        for (size_t i = 0; i < s->str_operands.count; i++)
            visit_Expr(arena, s->str_operands.at[i]);
        return;
    }
    case NODE_StmtDBDecl: {
        StmtDBDecl *s = (StmtDBDecl *)stmt;
        for (size_t i = 0; i < s->fields.count; i++)
            visit_Stmt(arena, (Stmt *)s->fields.at[i]);
        return;
    }
    case NODE_StmtExpr: {
        StmtExpr *s = (StmtExpr *)stmt;
        visit_Expr(arena, s->expr);
        return;
    }
    
    default: UNREACHABLE;
    }
}

void constexpr_pass(ProgramAST *ast, Arena *arena) {
    for (size_t i = 0; i < ast->stmts.count; i++)
        visit_Stmt(arena, ast->stmts.at[i]);
}