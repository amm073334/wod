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
    bool is_const;
    int32_t const_int;
    std::string const_str;
};

// statements
struct FunctionStmt : public Stmt {
    FunctionStmt(Position pos, WodType return_type, std::string name, std::vector<VarStmt*> params, std::vector<Stmt*> body, bool is_inline)
        : Stmt(pos), return_type(return_type), name(name), params(params), body(body), is_inline(is_inline) {}
    void accept(Visitor* v) override { v->visit_FunctionStmt(this); }
    WodType return_type;
    std::string name;
    std::vector<VarStmt*> params;
    std::vector<Stmt*> body;
    Symbol* sym;
    bool is_inline;
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
    VarStmt(Position pos, bool is_const, WodType type, std::string name, Expr* initializer)
        : Stmt(pos), is_const(is_const), type(type), name(name), initializer(initializer) {}
    void accept(Visitor* v) override { v->visit_VarStmt(this); }
    bool is_const;
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

struct ContinueStmt : public Stmt {
    ContinueStmt(Position pos)
        : Stmt(pos) {}
    void accept(Visitor* v) override { v->visit_ContinueStmt(this); }
};

struct BreakStmt : public Stmt {
    BreakStmt(Position pos)
        : Stmt(pos) {}
    void accept(Visitor* v) override { v->visit_BreakStmt(this); }
};

struct CmdStmt : public Stmt {
    CmdStmt(Position pos, Expr* cmd_id, std::vector<Expr*> int_fields, std::vector<Expr*> str_fields)
        : Stmt(pos), cmd_id(cmd_id), int_fields(int_fields), str_fields(str_fields) {}
    void accept(Visitor* v) override { v->visit_CmdStmt(this); }
    Expr* cmd_id;
    std::vector<Expr*> int_fields;
    std::vector<Expr*> str_fields;
};


// expressions
struct AssignExpr : public Expr {
    AssignExpr(Position pos, Expr* lhs, Expr* rhs)
        : Expr(pos), lhs(lhs), rhs(rhs) {}
    void accept(Visitor* v) override { v->visit_AssignExpr(this); }
    Expr* lhs;
    Expr* rhs;
};

struct VariableExpr : public Expr {
    VariableExpr(Position pos, std::string name)
        : Expr(pos), name(name) {}
    void accept(Visitor* v) override { v->visit_VariableExpr(this); }
    std::string name;
    Symbol* sym;
};

struct BinaryExpr : public Expr {
    enum BinaryOp {
        LOGIC_AND,
        LOGIC_OR,
        BIT_AND,
        BIT_OR,
        BIT_XOR,
        EQ,
        NEQ,
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
        MODULO
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
    Symbol* sym;
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

struct FStringExpr : public Expr {
    struct Fragment {
        std::string str;
        Expr* expr;
    };
    FStringExpr(Position pos, std::vector<Fragment> frags)
        : Expr(pos), frags(frags) {}
    void accept(Visitor* v) override { v->visit_FStringExpr(this); }
    std::vector<Fragment> frags;
};