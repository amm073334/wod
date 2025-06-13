#pragma once

#include <vector>
#include "token.h"
#include "visitor.h"
#include "environment.h"

struct Stmt { virtual void accept(Visitor* v) = 0; };
struct Expr { virtual void accept(Visitor* v) = 0; WodType type; };

// statements
struct FunctionStmt : public Stmt {
    struct ParamDecl {
        Token* type;
        Token* name;
    };
    FunctionStmt(Token* return_type, Token* name, std::vector<ParamDecl> params, std::vector<Stmt*> body)
        : return_type(return_type), name(name), params(params), body(body) {}
    void accept(Visitor* v) override { v->visit_FunctionStmt(this); }
    Token* return_type;
    Token* name;
    std::vector<ParamDecl> params;
    std::vector<Stmt*> body;
    Symbol* sym;
};

struct BlockStmt : public Stmt {
    BlockStmt(std::vector<Stmt*> stmts)
        : stmts(stmts) {}
    void accept(Visitor* v) override { v->visit_BlockStmt(this); }
    std::vector<Stmt*> stmts;
};

struct ReturnStmt : public Stmt {
    ReturnStmt(Token* keyword, Expr* expr)
        : keyword(keyword), expr(expr) {}
    void accept(Visitor* v) override { v->visit_ReturnStmt(this); }
    Token* keyword;
    Expr* expr;
};

struct VarStmt : public Stmt {
    VarStmt(std::vector<Token*> qualifiers, Token* name, Expr* initializer)
        : qualifiers(qualifiers), name(name), initializer(initializer) {}
    void accept(Visitor* v) override { v->visit_VarStmt(this); }
    std::vector<Token*> qualifiers;
    Token* name;
    Expr* initializer;
    Symbol* sym;
};

struct AssignStmt : public Stmt {
    AssignStmt(Token* name, Expr* expr)
        : name(name), expr(expr) {}
    void accept(Visitor* v) override { v->visit_AssignStmt(this); }
    Token* name;
    Expr* expr;
    Environment* env;
};

struct ExprStmt : public Stmt {
    ExprStmt(Expr* expr)
        : expr(expr) {}
    void accept(Visitor* v) override { v->visit_ExprStmt(this); }
    Expr* expr;
};

struct IfStmt : public Stmt {
    IfStmt(Token* keyword, Expr* condition, Stmt* then_branch, Stmt* else_branch)
        : keyword(keyword), condition(condition), then_branch(then_branch), else_branch(else_branch) {}
    void accept(Visitor* v) override { v->visit_IfStmt(this); }
    Token* keyword;
    Expr* condition;
    Stmt* then_branch;
    Stmt* else_branch;
};

struct LoopStmt : public Stmt {
    LoopStmt(Token* keyword, Expr* count, Stmt* body)
        : keyword(keyword), count(count), body(body) {}
    void accept(Visitor* v) override { v->visit_LoopStmt(this); }
    Token* keyword;
    Expr* count;
    Stmt* body;
};


// expressions
struct AssignExpr : public Expr {
    AssignExpr(Token* name, Expr* expr)
        : name(name), expr(expr) {}
    void accept(Visitor* v) override { v->visit_AssignExpr(this); }
    Token* name;
    Expr* expr;
    Environment* env;
};

struct VariableExpr : public Expr {
    VariableExpr(Token* name)
        : name(name) {}
    void accept(Visitor* v) override { v->visit_VariableExpr(this); }
    Token* name;
    Environment* env;
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
    Environment* env;
};

struct IntLiteralExpr : public Expr {
    IntLiteralExpr(int32_t value)
        : value(value) {}
    void accept(Visitor* v) override { v->visit_IntLiteralExpr(this); }
    int32_t value;
};

struct StrLiteralExpr : public Expr {
    StrLiteralExpr(std::string value)
        : value(value) {}
    void accept(Visitor* v) override { v->visit_StrLiteralExpr(this); }
    std::string value;
};