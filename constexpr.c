#include "constexpr.h"

ExprIntLit *make_int(Arena *arena, Token tok) {
    ExprIntLit *out = arena_alloc_assert(arena, sizeof(ExprIntLit));
    out->base.loc = tok; \
    out->base.kind = NODE_ExprIntLit;
    out->base.type = (WodType){
        .basetype = TYPE_INT,
        .is_assignable = false,
        .is_compile_time = true
    };
}

static void visit_Expr(Expr *expr, Arena *arena) {
    switch (expr->kind) {
    case NODE_ExprVar:
    case NODE_ExprArray:
    case NODE_ExprAccess:
    case NODE_ExprBinary: {
        ExprBinary *e = (ExprBinary *)expr;
        if (!e->left->type.is_compile_time
            || !e->right->type.is_compile_time)
            return;

        switch (e->op.type) {
        case TOK_PLUS: {
            ExprIntLit *merged = make_int(arena, e->base.loc);
        }
            
        default: UNREACHABLE;
        }
    }
    case NODE_ExprUnary:
    case NODE_ExprCall:
    case NODE_ExprIntLit:
    case NODE_ExprStrLit:
    case NODE_ExprBoolLit:
    default: UNREACHABLE;
    }
}

static void visit_Stmt(Stmt *stmt) {
    switch (stmt->kind) {}
}

void constexpr_pass(ProgramAST *ast, Arena *arena) {
    for (size_t i = 0; i < ast->stmts.count; i++) {
        visit_Stmt(ast->stmts.at[i]);
    }
}