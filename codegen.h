#pragma once

#include <iostream>
#include <stack>
#include "visitor.h"
#include "ast.h"
#include "commonevent.h"
#include "command.h"

class Codegen : public Visitor {
public:
    std::string gen(std::vector<Stmt*> &stmts) {
        for (Stmt* s : stmts) {
            if (!s) error("null statement");
            s->accept(this);
        }

        std::string out =
            "[COMMON_EVENT_TEXT_OUTPUT]\nCOMMON_EVENT_NUM=" + std::to_string(cevs.size()) + "\n";
        for (CommonEvent &cev : cevs) {
            out += "--------------------------\n";
            out += cev.to_string();
        }
        return out;
    }

private:
    struct Value {
        const int32_t v;
        const bool is_ref;
    };

    static const int32_t VAR_THRESHOLD = 1000000;
    static const int32_t CSELF_THRESHOLD = 1600000;
    std::vector<CommonEvent> cevs;
    CommonEvent* current_cev;
    int32_t int_stack_pos = CSELF_THRESHOLD + 10;

    std::stack<Value> eval_stack;
    Value pop() { Value tmp = eval_stack.top(); eval_stack.pop(); return tmp; }

    // statements
    void visit_FunctionStmt(FunctionStmt* stmt) override {
        cevs.push_back(CommonEvent());
        current_cev = &cevs.back();
        
        for (Stmt* s : stmt->body) {
            s->accept(this);
        }

        if (current_cev->COMMAND_NUM == 0) {
            current_cev->add_cmd(CMD_EMPTY);
        }
    }

    void visit_BlockStmt(BlockStmt* stmt) override {
        for (Stmt* s : stmt->stmts) {
            s->accept(this);
        }
    }

    void visit_ReturnStmt(ReturnStmt* stmt) override {
        if (!stmt->expr) {
            current_cev->add_cmd(CMD_RETURN);
        } else {
            stmt->expr->accept(this);
            if (current_cev->RETURN_VAL_TARGET == -1) {
                current_cev->RETURN_VAL_TARGET = int_stack_pos - CSELF_THRESHOLD;
            }
            add_arith(Value{CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET, true}, pop(), Value{0, false});
            current_cev->add_cmd(CMD_RETURN);
        }
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        stmt->expr->accept(this);
    }

    void visit_VarStmt(VarStmt* stmt) override {
        stmt->initializer->accept(this);
    }

    // expressions
    void visit_VariableExpr(VariableExpr* expr) override {
    }

    void visit_BinaryExpr(BinaryExpr* expr) override {
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        expr->right->accept(this);
        switch (expr->op->token_type) {
            case T_BANG:
                error("T_BANG wip");
                break;
            case T_MINUS:
                Value right = pop();
                if (right.is_ref) {
                    add_arith(push_new_temp(), Value{0, false}, right, ARITH_OP_MINUS);
                }
                eval_stack.push(Value{});
                break;
        }
    }

    void visit_CallExpr(CallExpr* expr) override {
        for (Expr* arg : expr->args) {
            arg->accept(this);
        }
        current_cev->add_cmd(CMD_CALLNAME, {0, 0}, {expr->name->text});
    }

    void visit_LiteralExpr(LiteralExpr* expr) override {
        eval_stack.push(Value{expr->value.n, false});
    }

    // utility
    Value push_new_temp() {
        eval_stack.push(new_temp());
        return eval_stack.top();
    }

    Value new_temp() {
        return Value{++int_stack_pos, true};
    }

    void add_arith(Value lhs, Value rhs_0, Value rhs_1) {
        add_arith(lhs, rhs_0, rhs_1, ARITH_OP_PLUS);
    }

    void add_arith(Value lhs, Value rhs_0, Value rhs_1, ArithFlags flag) {
        bool suppress_rhs_0 = rhs_0.v >= VAR_THRESHOLD && !rhs_0.is_ref;
        bool suppress_rhs_1 = rhs_1.v >= VAR_THRESHOLD && !rhs_1.is_ref;
        int32_t flags = flag
            | suppress_rhs_0 ? ARITH_FLAG_SUPPRESS_RHS_0 : 0
            | suppress_rhs_1 ? ARITH_FLAG_SUPPRESS_RHS_1 : 0;
        current_cev->add_cmd(CMD_ARITH, {lhs.v, rhs_0.v, rhs_1.v, flags}, {});
    }

    void error(std::string error_msg) {
        throw std::runtime_error(error_msg);
    }
};