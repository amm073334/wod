#pragma once

#include <iostream>
#include <stdexcept>
#include <stack>
#include <cassert>
#include "visitor.h"
#include "ast.h"
#include "environment.h"

class Typechecker : public Visitor {
public:
    Environment typecheck(std::vector<Stmt*> &program) {
        for (Stmt* s : program) s->accept(this);
        globals_visited = true;
        for (Stmt* s : program) s->accept(this);
        assert(&global_env == current_env);
        return global_env;
    }

    bool failed() { return had_error; }

    // statements
    void visit_FunctionStmt(FunctionStmt* stmt) override {
        if (!globals_visited) define_function(stmt);
        else function_block(stmt);
    }

    void define_function(FunctionStmt* stmt) {
        std::vector<WodType> arg_types;
        for (FunctionStmt::ParamDecl param : stmt->params) {
            arg_types.push_back(param.type);
        }
        if (current_cev_ref > MAX_CEV_REF) error(stmt->pos, "Maximum number of functions exceeded");
        stmt->sym = current_env->define_function(stmt->name, current_cev_ref, stmt->return_type, arg_types);
        if (!stmt->sym) error(stmt->pos, "Function redeclaration");
    }

    void function_block(FunctionStmt* stmt) {
        current_return_type = current_env->get(stmt->name)->type;
        open_scope();
        int32_t int_param_ref = CSELF_THRESHOLD;
        int32_t str_param_ref = CSELF_THRESHOLD + 5;
        for (FunctionStmt::ParamDecl param : stmt->params) {
            switch (param.type) {
                case T_INT: current_env->define(param.name, TYPE_INT)->ref = int_param_ref++; break;
                case T_STR: current_env->define(param.name, TYPE_STR)->ref = str_param_ref++; break;
            }
        }
        for (Stmt* s : stmt->body) s->accept(this);
        close_scope();
    }

    void visit_BlockStmt(BlockStmt* stmt) override {
        open_scope();
        for (Stmt* s : stmt->stmts) s->accept(this);
        close_scope();
    }

    void visit_ReturnStmt(ReturnStmt* stmt) override {
        stmt->expr->accept(this);
        if (stmt->expr->type != current_return_type)
            error(stmt->pos, "Return type mismatch");
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        stmt->expr->accept(this);
    }

    void visit_VarStmt(VarStmt* stmt) override {
        stmt->initializer->accept(this);
        // only one qualifier for now
        if (stmt->initializer->type != stmt->type)
            error(stmt->pos, "Declaration type mismatch");

        stmt->sym = current_env->define(stmt->name, stmt->type);
    }

    void visit_IfStmt(IfStmt* stmt) override {
        stmt->condition->accept(this);
        if (stmt->condition->type != TYPE_INT)
            error(stmt->pos, "Branch condition is not an integer");
        open_scope();
        stmt->then_branch->accept(this);
        close_scope();
        if (stmt->else_branch) {
            open_scope();
            stmt->else_branch->accept(this);
            close_scope();
        }
    }

    void visit_LoopStmt(LoopStmt* stmt) override {
        if (stmt->count) {
            stmt->count->accept(this);
            if (stmt->count->type != TYPE_INT)
                error(stmt->pos, "Loop count is not an integer");
        }
        open_scope();
        stmt->body->accept(this);
        close_scope();
    }

    // expressions
    void visit_AssignExpr(AssignExpr* expr) override {
        expr->env = current_env;
        expr->lhs->accept(this);
        expr->rhs->accept(this);

        if (!expr->lhs->assignable)
            error(expr->pos, "Attempted to assign to non-variable expression");
        if (expr->lhs->type != expr->rhs->type)
            error(expr->pos, "Assignment type mismatch");
        expr->type = expr->lhs->type;
        expr->assignable = false;
    }

    void visit_VariableExpr(VariableExpr* expr) override {
        expr->env = current_env;
        Symbol* sym = current_env->get(expr->name);
        if (!sym) error(expr->pos, "Variable not declared");
        expr->type = sym->type;
        expr->assignable = true;
    }

    void visit_BinaryExpr(BinaryExpr* expr) override {
        expr->left->accept(this);
        expr->right->accept(this);

        if (expr->left->type != TYPE_INT || expr->right->type != TYPE_INT)
            error(expr->pos, "Integer arguments expected");
        expr->type = TYPE_INT;
        expr->assignable = false;
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        expr->right->accept(this);
        if (expr->right->type != TYPE_INT)
            error(expr->pos, "Integer argument expected");
        expr->type = TYPE_INT;
        expr->assignable = false;
    }

    void visit_CallExpr(CallExpr* expr) override {
        expr->env = current_env;
        Symbol* sym = current_env->get(expr->name);
        if (!sym) error(expr->pos, "Function not declared");
        if (expr->args.size() != sym->arg_types.size())
            error(expr->pos, "Wrong number of arguments");
        for (size_t i = 0; i < sym->arg_types.size(); i++) {
            expr->args.at(i)->accept(this);
            if (expr->args.at(i)->type != sym->arg_types.at(i))
                error(expr->pos, "Type mismatch in argument list");
        }
        expr->type = sym->type;
        expr->assignable = false;
    }
    
    void visit_IntLiteralExpr(IntLiteralExpr* expr) override {
        expr->type = TYPE_INT;
        expr->assignable = false;
    }

    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {
        expr->type = TYPE_STR;
        expr->assignable = false;
    }

private:
    bool had_error = false;
    bool globals_visited = false;

    int32_t current_cev_ref = CEV_THRESHOLD;
    int32_t current_cint_ref = CSELF_THRESHOLD + 10;

    Environment global_env;
    Environment* current_env = &global_env;
    void open_scope() { current_env = new Environment; }
    void close_scope() { current_env = current_env->parent(); assert(current_env); }

    WodType current_return_type;

    // utility
    void error(Position pos, std::string error_msg) {
        had_error = true;
        std::cout << "typecheck error: line " << pos.line << " col " << pos.col << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};