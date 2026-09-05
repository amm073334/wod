#ifndef WOD_PARSER_H_
#define WOD_PARSER_H_

#include "common.h"
#include "lexer.h"
#include "environment.h"
#include "module.h"

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
    NODE_ExprInterp,
    NODE_ExprStructLitField,
    NODE_ExprDBDataElem,
    // NODE_ExprArrayLit,
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    Token tok;

    // For use during/after typechecking.
    WodType type;
    Environment* env;
} Expr;
VEC_PTR_DEF(Expr);

typedef enum {
    NODE_StmtAssign,
    NODE_StmtVarDecl,
    NODE_StmtCevDecl,
    NODE_StmtBlock,
    NODE_StmtReturn,
    NODE_StmtIf,
    NODE_StmtLoop,
    NODE_StmtForC,
    NODE_StmtForRange,
    NODE_StmtContinue,
    NODE_StmtBreak,
    NODE_StmtCmd,
    NODE_StmtCall,
    NODE_StmtInc,
    NODE_StmtDec,
    NODE_StmtDefDB,
    // NODE_StmtDefSpace,
    NODE_StmtDBTypeDecl,
    NODE_StmtDBDataDecl,
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    Token tok;

    // For use during/after typechecking.
    Environment *env;
} Stmt;
VEC_PTR_DEF(Stmt);

typedef struct Import {
    Token tok;
    StringView path;
} Import;
VEC_DEF(Import);

typedef struct ProgramAST {
    StringView apply;
    VEC_Import imports;
    VEC_PTR_Stmt stmts;
} ProgramAST;
VEC_PTR_DEF(ProgramAST);

typedef struct {
    Expr base;
    StringView name;

    Symbol *sym;
} ExprVar;
VEC_PTR_DEF(ExprVar);

typedef struct {
    Expr base;
    Expr *left;
    Expr *index;
} ExprArray;

typedef struct {
    Expr base;
    Expr *left;
    Token name;

    Symbol *sym;
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
    Expr base;
    StringView name;
    Expr *value;
} ExprStructLitField;
VEC_PTR_DEF(ExprStructLitField);

typedef struct {
    Expr base;
    StringView name;
    VEC_PTR_ExprStructLitField fields;
    bool no_body;
} ExprDBDataElem;
VEC_PTR_DEF(ExprDBDataElem);

typedef struct ExprInterp ExprInterp;
struct ExprInterp {
    Expr base;
    StringView opening;
    Expr *expr;
    
    // Should either be a string literal or
    // another interpolation node.
    Expr *next;
};

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

    Symbol *sym;
} StmtVarDecl;

VEC_PTR_DEF(StmtVarDecl);

typedef struct {
    Stmt base;
    Token ret;
    StringView name;
    VEC_PTR_StmtVarDecl params;
    VEC_PTR_Stmt body;
    bool is_exaddr;

    Symbol *sym;
} StmtCevDecl;

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
    Stmt *init;
    Expr *condition;
    Stmt *iter_stmt;
    Stmt *body;
} StmtForC;

typedef struct {
    Stmt base;
    StmtVarDecl *decl;
    Expr *right_bound;
    Stmt *body;
} StmtForRange;

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
    ExprCall *call;
} StmtCall;

typedef struct {
    Stmt base;
    Expr *expr;
} StmtInc;

typedef struct {
    Stmt base;
    Expr *expr;
} StmtDec;

VEC_DEF(Token);
typedef struct {
    Stmt base;
    DBKind db;
    VEC_Token db_types;

    VEC_PTR_Symbol symbols;
} StmtDefDB;

typedef struct {
    Stmt base;
    Token db;
    StringView name;
    VEC_PTR_StmtVarDecl fields;
    VEC_PTR_ExprDBDataElem data;

    Symbol *sym;
} StmtDBTypeDecl;

typedef struct {
    Stmt base;
    Token db_type;
    ExprDBDataElem *data;

    Symbol *dbtype_sym;
    Symbol *data_sym;
} StmtDBDataDecl;

// Return a topological sort of all modules, or an empty vector
// if an error occurred.
VEC_Module parse_all_modules(StringView path, Arena *arena);

#endif // WOD_PARSER_H_