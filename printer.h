#pragma once

#include <iostream>
#include "visitor.h"
#include "ast.h"

class Printer : Visitor {
public:
    void print(std::vector<Stmt*> &stmts) {
        for (Stmt* s : stmts) {
            s->accept(this);
        }
    }

    // void visit_FunctionStmt(FunctionStmt* stmt) override {
    //     std::cout << stmt->name->to_string() << "(";
    //     for (Token* t : stmt->params) {
    //         std::cout << t->to_string() << " ";
    //     }
    //     std::cout << ")" << std::endl;
    //     for (Stmt* s : stmt->body) {
    //         s->accept(this);
    //     }
    // }

    // void visit_BlockStmt(BlockStmt* stmt) override {
    //     for (Stmt* s : stmt->stmts) {
    //         s->accept(this);
    //     }
    // }

    // void visit_ReturnStmt(ReturnStmt* stmt) override {
    //     std::cout << "return" << std::endl;
    // }

    // void visit_LiteralExpr(LiteralExpr* expr) override {
    //     std::cout << "(" << expr->value.n << ")" << std::endl;
    // }
};