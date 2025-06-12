#pragma once

struct FunctionStmt;
struct BlockStmt;
struct ReturnStmt;
struct ExprStmt;
struct VarStmt;
struct AssignStmt;
struct IfStmt;
struct LoopStmt;

struct LiteralExpr;
struct VariableExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;

class Visitor {
public:
    // statements
    virtual void visit_FunctionStmt(FunctionStmt* stmt) = 0;
    virtual void visit_BlockStmt(BlockStmt* stmt) = 0;
    virtual void visit_ReturnStmt(ReturnStmt* stmt) = 0;
    virtual void visit_ExprStmt(ExprStmt* stmt) = 0;
    virtual void visit_VarStmt(VarStmt* stmt) = 0;
    virtual void visit_AssignStmt(AssignStmt* stmt) = 0;
    virtual void visit_IfStmt(IfStmt* stmt) = 0;
    virtual void visit_LoopStmt(LoopStmt* stmt) = 0;

    // expressions
    virtual void visit_LiteralExpr(LiteralExpr* expr) = 0;
    virtual void visit_VariableExpr(VariableExpr* expr) = 0;
    virtual void visit_BinaryExpr(BinaryExpr* expr) = 0;
    virtual void visit_UnaryExpr(UnaryExpr* expr) = 0;
    virtual void visit_CallExpr(CallExpr* expr) = 0;
};