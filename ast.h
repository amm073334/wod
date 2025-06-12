#pragma once

#include <vector>
#include "token.h"
#include "types.h"
#include "visitor.h"

struct Stmt { virtual void accept(Visitor* v) = 0; };
struct Expr { virtual void accept(Visitor* v) = 0; };

// statements
struct FunctionStmt : public Stmt {
    FunctionStmt(Token* name, std::vector<Token*> params, std::vector<Stmt*> body)
        : name(name), params(params), body(body) {}
    void accept(Visitor* v) override { v->visit_FunctionStmt(this); }
    Token* name;
    std::vector<Token*> params;
    std::vector<Stmt*> body;
};

struct BlockStmt : public Stmt {
    BlockStmt(std::vector<Stmt*> stmts)
        : stmts(stmts) {}
    void accept(Visitor* v) override { v->visit_BlockStmt(this); }
    std::vector<Stmt*> stmts;
};

struct ReturnStmt : public Stmt {
    ReturnStmt(Expr* expr)
        : expr(expr) {}
    void accept(Visitor* v) override { v->visit_ReturnStmt(this); }
    Expr* expr;
};

struct VarStmt : public Stmt {
    VarStmt(Token* name, Expr* initializer)
        : name(name), initializer(initializer) {}
    void accept(Visitor* v) override { v->visit_VarStmt(this); }
    Token* name;
    Expr* initializer;
};

struct AssignStmt : public Stmt {
    AssignStmt(Token* name, Expr* expr)
        : name(name), expr(expr) {}
    void accept(Visitor* v) override { v->visit_AssignStmt(this); }
    Token* name;
    Expr* expr;
};

struct ExprStmt : public Stmt {
    ExprStmt(Expr* expr)
        : expr(expr) {}
    void accept(Visitor* v) override { v->visit_ExprStmt(this); }
    Expr* expr;
};

struct IfStmt : public Stmt {
    IfStmt(Expr* condition, Stmt* then_branch, Stmt* else_branch)
        : condition(condition), then_branch(then_branch), else_branch(else_branch) {}
    void accept(Visitor* v) override { v->visit_IfStmt(this); }
    Expr* condition;
    Stmt* then_branch;
    Stmt* else_branch;
};

struct LoopStmt : public Stmt {
    LoopStmt(Expr* count, Stmt* body)
        : count(count), body(body) {}
    void accept(Visitor* v) override { v->visit_LoopStmt(this); }
    Expr* count;
    Stmt* body;
};


// expressions
struct VariableExpr : public Expr {
    VariableExpr(Token* name)
        : name(name) {}
    void accept(Visitor* v) override { v->visit_VariableExpr(this); }
    Token* name;
};

struct BinaryExpr : public Expr {
    BinaryExpr(Expr* left, Token* op, Expr* right)
        : left(left), op(op), right(right) {}
    void accept(Visitor* v) override { v->visit_BinaryExpr(this); }
    Expr* left;
    Token* op;
    Expr* right;
};

struct UnaryExpr : public Expr {
    UnaryExpr(Token* op, Expr* right)
        : op(op), right(right) {}
    void accept(Visitor* v) override { v->visit_UnaryExpr(this); }
    Token* op;
    Expr* right;
};

struct CallExpr : public Expr {
    CallExpr(Token* name, std::vector<Expr*> args)
        : name(name), args(args) {}
    void accept(Visitor* v) override { v->visit_CallExpr(this); }
    Token* name;
    std::vector<Expr*> args;
};

struct LiteralExpr : public Expr {
    LiteralExpr(LitVal value)
        : value(value) {}
    void accept(Visitor* v) override { v->visit_LiteralExpr(this); }
    LitVal value;
};