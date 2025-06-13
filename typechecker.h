#pragma once

#include <iostream>
#include <stdexcept>
#include <stack>
#include <cassert>
#include "visitor.h"
#include "ast.h"
#include "environment.h"

class Typechecker : Visitor {
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
        WodType return_type;
        switch (stmt->return_type->token_type) {
            case T_VOID: return_type = TYPE_VOID; break;
            case T_INT: return_type = TYPE_INT; break;
            case T_STR: return_type = TYPE_STR; break;
            default: error(stmt->return_type, "Invalid return type"); break;
        }
        std::vector<WodType> arg_types;
        for (FunctionStmt::ParamDecl param : stmt->params) {
            switch (param.type->token_type) {
                case T_INT: arg_types.push_back(TYPE_INT); break;
                case T_STR: arg_types.push_back(TYPE_STR); break;
                default: error(param.type, "Invalid parameter type"); break;
            }
        }
        if (current_cev_ref > MAX_CEV_REF) error(stmt->name, "Maximum number of functions exceeded");
        stmt->sym = current_env->define_function(stmt->name->text, current_cev_ref, return_type, arg_types);
        if (!stmt->sym) error(stmt->name, "Function redeclaration");
    }

    void function_block(FunctionStmt* stmt) {
        current_return_type = current_env->get(stmt->name->text)->type;
        open_scope();
        int32_t int_param_ref = CSELF_THRESHOLD;
        int32_t str_param_ref = CSELF_THRESHOLD + 5;
        for (FunctionStmt::ParamDecl param : stmt->params) {
            switch (param.type->token_type) {
                case T_INT: current_env->define(param.name->text, TYPE_INT)->ref = int_param_ref++; break;
                case T_STR: current_env->define(param.name->text, TYPE_STR)->ref = str_param_ref++; break;
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
            error(stmt->keyword, "Return type mismatch");
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        stmt->expr->accept(this);
    }

    void visit_VarStmt(VarStmt* stmt) override {
        stmt->initializer->accept(this);
        // only one qualifier for now
        WodType lhs_type;
        switch (stmt->qualifiers.at(0)->token_type) {
            case T_INT:
                lhs_type = TYPE_INT;
            case T_STR:
                lhs_type = TYPE_STR;
            default: error(stmt->qualifiers.at(0), "Invalid variable type"); break;
        }
        if (stmt->initializer->type != lhs_type)
            error(stmt->qualifiers.at(0), "Declaration type mismatch");

        stmt->sym = current_env->define(stmt->name->text, lhs_type);
    }

    void visit_AssignStmt(AssignStmt* stmt) override {
        stmt->env = current_env;
        stmt->expr->accept(this);
        Symbol* sym = current_env->get(stmt->name->text);
        if (!sym) error(stmt->name, "Variable not declared");
        if (stmt->expr->type != sym->type) error(stmt->name, "Assignment type mismatch");
    }

    void visit_IfStmt(IfStmt* stmt) override {
        stmt->condition->accept(this);
        if (stmt->condition->type != TYPE_INT) error(stmt->keyword, "Branch condition is not an integer");
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
            if (stmt->count->type != TYPE_INT) error(stmt->keyword, "Loop count is not an integer");
        }
        open_scope();
        stmt->body->accept(this);
        close_scope();
    }

    // expressions
    void visit_VariableExpr(VariableExpr* expr) override {
        expr->env = current_env;
        Symbol* sym = current_env->get(expr->name->text);
        if (!sym) error(expr->name, "Variable not declared");
        expr->type = sym->type;
    }

    void visit_BinaryExpr(BinaryExpr* expr) override {
        expr->left->accept(this);
        expr->right->accept(this);

        switch (expr->op->token_type) {
            case T_PIPE_PIPE:
            case T_AMP_AMP:
            case T_PIPE:
            case T_AMP:
            case T_GREATER:
            case T_GREATER_EQUAL:
            case T_LESS:
            case T_LESS_EQUAL:
            case T_LESS_LESS:
            case T_GREATER_GREATER:
            case T_PLUS:
            case T_MINUS:
            case T_STAR:
            case T_SLASH:
                if (expr->left->type != TYPE_INT || expr->right->type != TYPE_INT)
                    error(expr->op, "Integer arguments expected");
                expr->type = TYPE_INT;
                break;
        }
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        expr->right->accept(this);
        if (expr->right->type != TYPE_INT) error(expr->op, "Integer argument expected");
        expr->type = TYPE_INT;
    }

    void visit_CallExpr(CallExpr* expr) override {
        expr->env = current_env;
        Symbol* sym = current_env->get(expr->name->text);
        if (!sym) error(expr->name, "Function not declared");
        if (expr->args.size() != sym->arg_types.size()) error(expr->name, "Wrong number of arguments");
        for (size_t i = 0; i < sym->arg_types.size(); i++) {
            expr->args.at(i)->accept(this);
            if (expr->args.at(i)->type != sym->arg_types.at(i)) error(expr->name, "Type mismatch in argument list");
        }
        expr->type = sym->type;
    }
    
    void visit_IntLiteralExpr(IntLiteralExpr* expr) override {
        expr->type = TYPE_INT;
    }

    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {
        expr->type = TYPE_STR;
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
    void error(Token* t, std::string error_msg) {
        had_error = true;
        std::cout << "typecheck error: line " << t->line << " " << t->text << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};