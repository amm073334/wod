#include "visitor.h"
#include "ast.h"

class Inliner : public Visitor {
    // statements
    void visit_FunctionStmt(FunctionStmt* stmt) override {
        for (Stmt* s : stmt->body) s->accept(this);
    }

    void visit_BlockStmt(BlockStmt* stmt) override {
        for (Stmt* s : stmt->stmts) s->accept(this);
    }

    void visit_ReturnStmt(ReturnStmt* stmt) override {
        if (stmt->expr) stmt->expr->accept(this);
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        stmt->expr->accept(this);
    }

    void visit_VarStmt(VarStmt* stmt) override {
        if (stmt->initializer) stmt->initializer->accept(this);
    }

    void visit_IfStmt(IfStmt* stmt) override {
        stmt->condition->accept(this);
        stmt->then_branch->accept(this);
        stmt->else_branch->accept(this);
    }

    void visit_LoopStmt(LoopStmt* stmt) override {
        if (stmt->count) stmt->count->accept(this);
        stmt->body->accept(this);
    }

    void visit_CmdStmt(CmdStmt* stmt) override {
        for (Expr* e : stmt->int_fields) e->accept(this);
    }

    // expressions
    void visit_AssignExpr(AssignExpr* expr) override {
        expr->rhs->accept(this);
    }

    void visit_VariableExpr(VariableExpr* expr) override {}

    void visit_BinaryExpr(BinaryExpr* expr) override {
        expr->left->accept(this);
        expr->right->accept(this);
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        expr->right->accept(this);
    }

    void visit_CallExpr(CallExpr* expr) override {
        // if (!expr->sym->is_inline_function) return;
        for (Expr* arg : expr->args) arg->accept(this);
        
    }
    
    void visit_IntLiteralExpr(IntLiteralExpr* expr) override {}
    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {}
};