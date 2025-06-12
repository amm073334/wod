#pragma once

#include <iostream>
#include <stdexcept>
#include "visitor.h"
#include "ast.h"
#include "symboltable.h"

class Typechecker : public Visitor {
public:
    Typechecker(SymbolTable* st) : st(st) {}

private:
    SymbolTable* st;
    bool had_error = false;

    // statements
    void visit_FunctionStmt(FunctionStmt* stmt) override {}
    void visit_BlockStmt(BlockStmt* stmt) override {}
    void visit_ReturnStmt(ReturnStmt* stmt) override {}
    void visit_ExprStmt(ExprStmt* stmt) override {}
    void visit_VarStmt(VarStmt* stmt) override {}
    void visit_AssignStmt(AssignStmt* stmt) override {}
    void visit_IfStmt(IfStmt* stmt) override {}
    void visit_LoopStmt(LoopStmt* stmt) override {}

    // expressions
    void visit_LiteralExpr(LiteralExpr* expr) override {}
    void visit_VariableExpr(VariableExpr* expr) override {}
    void visit_BinaryExpr(BinaryExpr* expr) override {}
    void visit_UnaryExpr(UnaryExpr* expr) override {}
    void visit_CallExpr(CallExpr* expr) override {}

    // utility
    void error(Token* t, std::string error_msg) {
        had_error = true;
        std::cout << "typecheck error: line " << t->line << " " << t->text << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};