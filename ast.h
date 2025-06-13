#pragma once

#include <vector>
#include "token.h"
#include "visitor.h"
#include "environment.h"

struct Position {
    size_t line;
    size_t col;
};

struct Node {
    Node(Position pos) : pos(pos) {}
    virtual void accept(Visitor* v) = 0;
    Position pos;
};

struct Stmt : public Node {
    Stmt(Position pos) : Node(pos) {}
};

struct Expr : public Node {
    Expr(Position pos) : Node(pos) {}
    WodType type;
    bool assignable;
};

// statements
struct FunctionStmt : public Stmt {
    struct ParamDecl {
        WodType type;
        std::string name;
    };
    FunctionStmt(Position pos, WodType return_type, std::string name, std::vector<ParamDecl> params, std::vector<Stmt*> body)
        : Stmt(pos), return_type(return_type), name(name), params(params), body(body) {}
    void accept(Visitor* v) override { v->visit_FunctionStmt(this); }
    WodType return_type;
    std::string name;
    std::vector<ParamDecl> params;
    std::vector<Stmt*> body;
    Symbol* sym;
};

struct BlockStmt : public Stmt {
    BlockStmt(Position pos, std::vector<Stmt*> stmts)
        : Stmt(pos), stmts(stmts) {}
    void accept(Visitor* v) override { v->visit_BlockStmt(this); }
    std::vector<Stmt*> stmts;
};

struct ReturnStmt : public Stmt {
    ReturnStmt(Position pos, Expr* expr)
        : Stmt(pos), expr(expr) {}
    void accept(Visitor* v) override { v->visit_ReturnStmt(this); }
    Expr* expr;
};

struct VarStmt : public Stmt {
    VarStmt(Position pos, WodType type, std::string name, Expr* initializer)
        : Stmt(pos), type(type), name(name), initializer(initializer) {}
    void accept(Visitor* v) override { v->visit_VarStmt(this); }
    WodType type;
    std::string name;
    Expr* initializer;
    Symbol* sym;
};

struct ExprStmt : public Stmt {
    ExprStmt(Position pos, Expr* expr)
        : Stmt(pos), expr(expr) {}
    void accept(Visitor* v) override { v->visit_ExprStmt(this); }
    Expr* expr;
};

struct IfStmt : public Stmt {
    IfStmt(Position pos, Expr* condition, Stmt* then_branch, Stmt* else_branch)
        : Stmt(pos), condition(condition), then_branch(then_branch), else_branch(else_branch) {}
    void accept(Visitor* v) override { v->visit_IfStmt(this); }
    Expr* condition;
    Stmt* then_branch;
    Stmt* else_branch;
};

struct LoopStmt : public Stmt {
    LoopStmt(Position pos, Expr* count, Stmt* body)
        : Stmt(pos), count(count), body(body) {}
    void accept(Visitor* v) override { v->visit_LoopStmt(this); }
    Expr* count;
    Stmt* body;
};


// expressions
struct AssignExpr : public Expr {
    AssignExpr(Position pos, Expr* lhs, Expr* rhs)
        : Expr(pos), lhs(lhs), rhs(rhs) {}
    void accept(Visitor* v) override { v->visit_AssignExpr(this); }
    Expr* lhs;
    Expr* rhs;
    Environment* env;
};

struct VariableExpr : public Expr {
    VariableExpr(Position pos, std::string name)
        : Expr(pos), name(name) {}
    void accept(Visitor* v) override { v->visit_VariableExpr(this); }
    std::string name;
    Environment* env;
};

struct BinaryExpr : public Expr {
    enum BinaryOp {
        LOGIC_AND,
        LOGIC_OR,
        BIT_AND,
        BIT_OR,
        GT,
        GTE,
        LT,
        LTE,
        LSHIFT,
        RSHIFT,
        ADD,
        SUB,
        MUL,
        DIV,
    };
    BinaryExpr(Position pos, Expr* left, BinaryOp op, Expr* right)
        : Expr(pos), left(left), op(op), right(right) {}
    void accept(Visitor* v) override { v->visit_BinaryExpr(this); }
    Expr* left;
    BinaryOp op;
    Expr* right;
};

struct UnaryExpr : public Expr {
    enum UnaryOp {
        LOGIC_NOT,
        MINUS
    };
    UnaryExpr(Position pos, UnaryOp op, Expr* right)
        : Expr(pos), op(op), right(right) {}
    void accept(Visitor* v) override { v->visit_UnaryExpr(this); }
    UnaryOp op;
    Expr* right;
};

struct CallExpr : public Expr {
    CallExpr(Position pos, std::string name, std::vector<Expr*> args)
        : Expr(pos), name(name), args(args) {}
    void accept(Visitor* v) override { v->visit_CallExpr(this); }
    std::string name;
    std::vector<Expr*> args;
    Environment* env;
};

struct IntLiteralExpr : public Expr {
    IntLiteralExpr(Position pos, int32_t value)
        : Expr(pos), value(value) {}
    void accept(Visitor* v) override { v->visit_IntLiteralExpr(this); }
    int32_t value;
};

struct StrLiteralExpr : public Expr {
    StrLiteralExpr(Position pos, std::string value)
        : Expr(pos), value(value) {}
    void accept(Visitor* v) override { v->visit_StrLiteralExpr(this); }
    std::string value;
};