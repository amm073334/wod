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
    Environment* typecheck(std::vector<Stmt*> &program) {
        global_env = new Environment;
        current_env = global_env;
        for (Stmt* s : program) s->accept(this);
        globals_visited = true;
        for (Stmt* s : program) s->accept(this);
        assert(global_env == current_env);
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
        for (VarStmt* param : stmt->params) {
            arg_types.push_back(param->type);
        }
        if (stmt->is_inline) {
            stmt->sym = current_env->define_inline_function(stmt->name, stmt, stmt->return_type, arg_types);
        } else {
            if (current_cev_ref > MAX_CEV_REF) error(stmt->pos, "Maximum number of functions exceeded");
            stmt->sym = current_env->define_function(stmt->name, current_cev_ref++, stmt->return_type, arg_types);
        }
        if (!stmt->sym) error(stmt->pos, "Function redeclaration");
    }

    void function_block(FunctionStmt* stmt) {
        current_return_type = current_env->get(stmt->name)->type;
        if (stmt->is_inline) visiting_inline_function = true;
        open_scope();
        int32_t int_param_ref = CSELF_THRESHOLD;
        int32_t str_param_ref = CSELF_THRESHOLD + 5;
        for (VarStmt* param : stmt->params) {
            Symbol* sym;
            sym = current_env->define(param->name, param->type);
            if (!sym) error(stmt->pos, "Redeclaration of parameter");
            sym->initialized_var = true;
            switch (param->type) {
                case TYPE_INT: sym->ref = int_param_ref++; break;
                case TYPE_STR: sym->ref = str_param_ref++; break;
            }
            param->sym = sym;
        }
        for (Stmt* s : stmt->body) s->accept(this);
        close_scope();
        visiting_inline_function = false;
    }

    void visit_BlockStmt(BlockStmt* stmt) override {
        open_scope();
        for (Stmt* s : stmt->stmts) s->accept(this);
        close_scope();
    }

    void visit_ReturnStmt(ReturnStmt* stmt) override {
        WodType ret_expr_type;
        if (!stmt->expr) ret_expr_type = TYPE_VOID;
        else {
            stmt->expr->accept(this);
            ret_expr_type = stmt->expr->type;
        }
        if (ret_expr_type != current_return_type)
            error(stmt->pos, "Return type mismatch");
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        stmt->expr->accept(this);
    }

    void visit_VarStmt(VarStmt* stmt) override {
        if (!globals_visited) return;

        if (!stmt->initializer) {
            stmt->sym = current_env->define(stmt->name, stmt->type);
            stmt->sym->initialized_var = false;
        } else {
            stmt->initializer->accept(this);
            if (stmt->initializer->type != stmt->type)
                error(stmt->pos, "Declaration type mismatch");
            if (stmt->is_const) {
                if (!stmt->initializer->is_const)
                    error(stmt->pos, "Attempted to assign non-const expression to const variable");
                if (stmt->initializer->type == TYPE_INT)
                    stmt->sym = current_env->define_const_int(stmt->name, stmt->initializer->const_int);
                if (stmt->initializer->type == TYPE_STR)
                    stmt->sym = current_env->define_const_str(stmt->name, stmt->initializer->const_str);
            } else stmt->sym = current_env->define(stmt->name, stmt->type);
            stmt->sym->initialized_var = true;
        }
        if (!stmt->sym) error(stmt->pos, "Variable redeclaration");
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
        in_a_loop = true;
        stmt->body->accept(this);
        in_a_loop = false;
        close_scope();
    }

    void visit_ContinueStmt(ContinueStmt* stmt) override {
        if (!in_a_loop) error(stmt->pos, "Used continue statement outside of loop");
    }

    void visit_BreakStmt(BreakStmt* stmt) override {
        if (!in_a_loop) error(stmt->pos, "Used break statement outside of loop");
    }

    void visit_CmdStmt(CmdStmt* stmt) override {
        stmt->cmd_id->accept(this);
        if (!stmt->cmd_id->is_const) {
            error(stmt->pos, "cmd id must be a constant expression");
        }
        for (Expr* e : stmt->int_fields) {
            e->accept(this);
            if (e->type != TYPE_INT)
                error(e->pos, "Expected argument of integer type");
        }
        for (Expr* e : stmt->str_fields) {
            e->accept(this);
            if (e->type != TYPE_STR)
                error(e->pos, "Expected argument of string type");
        }
    }

    // expressions
    void visit_AssignExpr(AssignExpr* expr) override {
        expr->rhs->accept(this);

        if (visiting_assign_lhs)
            error(expr->pos, "Attempted to assign to non-variable expression");
        visiting_assign_lhs = true;
        expr->lhs->accept(this);
        visiting_assign_lhs = false;

        if (expr->lhs->is_const)
            error(expr->pos, "Attempted to assign to const variable");
        if (!expr->lhs->assignable)
            error(expr->pos, "Attempted to assign to non-variable expression");
        if (expr->lhs->type != expr->rhs->type)
            error(expr->pos, "Assignment type mismatch");
        
        expr->type = TYPE_VOID;
        expr->assignable = false;
        expr->is_const = false;
    }

    void visit_VariableExpr(VariableExpr* expr) override {
        Symbol* sym = current_env->get(expr->name);
        if (!sym) error(expr->pos, "Variable not declared");
        expr->sym = sym;

        if (visiting_assign_lhs)
            sym->initialized_var = true;
        // else if (!sym->initialized_var)
        //     error(expr->pos, "Accessed uninitialized variable");
        
        expr->type = sym->type;
        expr->assignable = !sym->is_const;
        expr->is_const = sym->is_const;
        if (expr->is_const) {
            if (expr->type == TYPE_INT) 
                expr->const_int = sym->ref;
            else
                expr->const_str = sym->const_string;
        }
    }

    void visit_BinaryExpr(BinaryExpr* expr) override {
        expr->left->accept(this);
        expr->right->accept(this);

        if (expr->left->type != expr->right->type)
            error(expr->pos, "Type mismatch in binary expression");
        else if (expr->left->type == TYPE_STR && expr->right->type == TYPE_STR){
            if (expr->op != BinaryExpr::EQ && expr->op != BinaryExpr::NEQ) {
                error(expr->pos, "Only comparison is supported between strings");
            }
        } else if (expr->left->type != TYPE_INT || expr->right->type != TYPE_INT)
            error(expr->pos, "Integer arguments expected");
        
        expr->type = TYPE_INT;
        expr->assignable = false;
        expr->is_const = expr->left->is_const && expr->right->is_const;
        if (expr->is_const) {
            switch (expr->op) {
                case BinaryExpr::LOGIC_AND:
                    expr->const_int = expr->left->const_int && expr->right->const_int;
                    break;
                case BinaryExpr::LOGIC_OR:
                    expr->const_int = expr->left->const_int || expr->right->const_int;
                    break;
                case BinaryExpr::BIT_AND:
                    expr->const_int = expr->left->const_int & expr->right->const_int;
                    break;
                case BinaryExpr::BIT_OR:
                    expr->const_int = expr->left->const_int | expr->right->const_int;
                    break;
                case BinaryExpr::BIT_XOR:
                    expr->const_int = expr->left->const_int ^ expr->right->const_int;
                    break;
                case BinaryExpr::GT:
                    expr->const_int = expr->left->const_int > expr->right->const_int;
                    break;
                case BinaryExpr::GTE:
                    expr->const_int = expr->left->const_int >= expr->right->const_int;
                    break;
                case BinaryExpr::LT:
                    expr->const_int = expr->left->const_int < expr->right->const_int;
                    break;
                case BinaryExpr::LTE:
                    expr->const_int = expr->left->const_int <= expr->right->const_int;
                    break;
                case BinaryExpr::LSHIFT:
                    expr->const_int = expr->left->const_int << expr->right->const_int;
                    break;
                case BinaryExpr::RSHIFT:
                    expr->const_int = expr->left->const_int >> expr->right->const_int;
                    break;
                case BinaryExpr::ADD:
                    expr->const_int = expr->left->const_int + expr->right->const_int;
                    break;
                case BinaryExpr::SUB:
                    expr->const_int = expr->left->const_int - expr->right->const_int;
                    break;
                case BinaryExpr::MUL:
                    expr->const_int = expr->left->const_int * expr->right->const_int;
                    break;
                case BinaryExpr::DIV:
                    expr->const_int = expr->left->const_int / expr->right->const_int;
                    break;
                case BinaryExpr::MODULO:
                    expr->const_int = expr->left->const_int % expr->right->const_int;
                    break;
                case BinaryExpr::EQ:
                    if (expr->left->type == TYPE_INT)
                        expr->const_int = expr->left->const_int == expr->right->const_int;
                    else
                        expr->const_int = expr->left->const_str == expr->right->const_str;
                    break;
                case BinaryExpr::NEQ:
                    if (expr->left->type == TYPE_INT)
                        expr->const_int = expr->left->const_int != expr->right->const_int;
                    else
                        expr->const_int = expr->left->const_str != expr->right->const_str;
                    break;
            }
        }
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        expr->right->accept(this);
        if (expr->right->type != TYPE_INT)
            error(expr->pos, "Integer argument expected");
        
        expr->type = TYPE_INT;
        expr->assignable = false;
        expr->is_const = expr->right->is_const;
        if (expr->is_const) {
            switch (expr->op) {
                case UnaryExpr::LOGIC_NOT:
                    expr->const_int = !expr->right->const_int;
                    break;
                case UnaryExpr::MINUS:
                    expr->const_int = -expr->right->const_int;
                    break;
            }
        }
    }

    void visit_CallExpr(CallExpr* expr) override {
        Symbol* sym = current_env->get(expr->name);
        if (visiting_inline_function && expr->sym->inline_function)
            error(expr->pos, "Calling an inline function from an inline function is currently unsupported");
        if (!sym) error(expr->pos, "Function not declared");
        expr->sym = sym;
        if (expr->args.size() != sym->arg_types.size())
            error(expr->pos, "Wrong number of arguments");
        for (size_t i = 0; i < sym->arg_types.size(); i++) {
            expr->args.at(i)->accept(this);
            if (expr->args.at(i)->type != sym->arg_types.at(i))
                error(expr->args.at(i)->pos, "Type mismatch in argument list");
        }
        
        expr->type = sym->type;
        expr->assignable = false;
        expr->is_const = false;
    }
    
    void visit_IntLiteralExpr(IntLiteralExpr* expr) override {
        expr->type = TYPE_INT;
        expr->assignable = false;
        expr->is_const = true;
        expr->const_int = expr->value;
    }

    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {
        expr->type = TYPE_STR;
        expr->assignable = false;
        expr->is_const = true;
        expr->const_str = expr->value;
    }

    void visit_FStringExpr(FStringExpr* expr) override {
        for (FStringExpr::Fragment &f : expr->frags) {
            if (f.expr) f.expr->accept(this);
        }
        expr->type = TYPE_STR;
        expr->assignable = false;
        expr->is_const = false;
    }

private:
    bool had_error = false;
    bool globals_visited = false;
    bool visiting_assign_lhs = false;
    bool in_a_loop = false;
    bool visiting_inline_function = false;

    int32_t current_cev_ref = CEV_THRESHOLD;
    int32_t current_cint_ref = CSELF_THRESHOLD + 10;

    Environment* global_env;
    Environment* current_env;
    void open_scope() { current_env = new Environment(current_env); }
    void close_scope() { current_env = current_env->parent(); assert(current_env); }

    WodType current_return_type;

    // utility
    void error(Position pos, std::string error_msg) {
        had_error = true;
        std::cout << "typecheck error: line " << pos.line << " col " << pos.col << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};