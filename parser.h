#ifndef WOD_PARSER_H_
#define WOD_PARSER_H_

#include "common.h"
#include "lexer.h"
#include "environment.h"

typedef enum {
    NODE_ExprVar,
    NODE_ExprArray,
    NODE_ExprAccess,
    NODE_ExprBinary,
    NODE_ExprUnary,
    NODE_ExprCall,
    NODE_ExprIntLit,
    NODE_ExprStrLit,
    NODE_ExprBoolLit,
    // NODE_ExprArrayLit,
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    Token loc;

    // For use during/after typechecking.
    WodType type;
    Environment* env;
} Expr;
VEC_PTR_DEF(Expr);

typedef enum {
    NODE_StmtAssign,
    NODE_StmtVarDecl,
    NODE_StmtFuncDecl,
    NODE_StmtBlock,
    NODE_StmtReturn,
    NODE_StmtIf,
    NODE_StmtLoop,
    NODE_StmtFor,
    NODE_StmtContinue,
    NODE_StmtBreak,
    NODE_StmtCmd,
    NODE_StmtDBDecl,
    NODE_StmtExpr,
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    Token loc;

    // For use during/after typechecking.
    Environment* env;
} Stmt;
VEC_PTR_DEF(Stmt);

typedef struct {
    Token loc;
    StringView path;

    // SV_NULL means unqualified.
    StringView alias;
} Import;
VEC_DEF(Import);

typedef struct {
    StringView file;
    const char *source;
    VEC_Import imports;
    VEC_PTR_Stmt stmts;
} ProgramAST;

typedef struct {
    Expr base;
    StringView name;
} ExprVar;

typedef struct {
    Expr base;
    Expr *left;
    Expr *index;
} ExprArray;

typedef struct {
    Expr base;
    Expr *left;
    Token name;
} ExprAccess;

typedef struct {
    Expr base;
    Token op;
    Expr *left;
    Expr *right;
} ExprBinary;

typedef struct {
    Expr base;
    Token op;
    Expr *right;
} ExprUnary;

typedef struct {
    Expr base;
    Expr *callee;
    VEC_PTR_Expr args;
} ExprCall;

typedef struct {
    Expr base;
    int32_t value;
} ExprIntLit;

typedef struct {
    Expr base;
    StringView value;
} ExprStrLit;

typedef struct {
    Expr base;
    bool value;
} ExprBoolLit;

typedef struct {
    Expr base;
    VEC_PTR_Expr value;
} ExprArrayLit;


typedef struct {
    Stmt base;
    Expr *left;
    Token assign_type;
    Expr *right;
} StmtAssign;

typedef struct {
    Stmt base;
    Token type;
    StringView name;
    Expr *array_length;
    Expr *initializer;
    bool is_const;
} StmtVarDecl;

VEC_PTR_DEF(StmtVarDecl);

typedef struct {
    Stmt base;
    Token ret;
    StringView name;
    VEC_PTR_StmtVarDecl params;
    VEC_PTR_Stmt body;
    bool is_inline;
} StmtFuncDecl;

typedef struct {
    Stmt base;
    VEC_PTR_Stmt stmts;
} StmtBlock;

typedef struct {
    Stmt base;
    Expr *expr;
} StmtReturn;

typedef struct {
    Stmt base;
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch;
} StmtIf;

typedef struct {
    Stmt base;
    Expr *count;
    Stmt *body;
} StmtLoop;

typedef struct {
    Stmt base;
    StringView iterator;
    Expr *left_bound;
    Expr *right_bound;
    Stmt *body;
} StmtFor;

typedef struct {
    Stmt base;
} StmtContinue;

typedef struct {
    Stmt base;
} StmtBreak;

typedef struct {
    Stmt base;
    Expr *id;
    VEC_PTR_Expr int_operands;
    VEC_PTR_Expr str_operands;
} StmtCmd;

typedef struct {
    Stmt base;
    Token db;
    StringView name;
    VEC_PTR_StmtVarDecl fields;
} StmtDBDecl;

typedef struct {
    Stmt base;
    Expr *expr;
} StmtExpr;

ProgramAST *generate_ast(StringView file_path, const char *source, Arena *arena);

#endif // WOD_PARSER_H_