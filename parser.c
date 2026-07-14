#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "error.h"

#define ALLOC_NODE(var, token, type_, ...) \
    do { \
        (var) = arena_alloc_assert(parser->arena, sizeof(type_)); \
        *var = __VA_ARGS__; \
        (var)->base.loc = (token); \
        (var)->base.kind = NODE_##type_; \
        (var)->base.env = NULL; \
    } while (0)

typedef struct {
    Lexer lexer;
    const char *source;
    StringView file_path;
    Arena *arena;
    Token previous;
    Token current;
    bool panic_mode;
    bool had_error;
} Parser;

static void parse_error(Parser *parser, Token *token, StringView message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;

    error(parser->file_path, parser->source, token, message);
}

static void error_previous(Parser *parser, StringView message) {
    parse_error(parser, &parser->previous, message);
}

static void error_current(Parser *parser, StringView message) {
    parse_error(parser, &parser->current, message);
}

static void advance(Parser *parser) {
    parser->previous = parser->current;
    for (;;) {
        parser->current = scan_token(&parser->lexer);
        if (parser->current.type != TOK_ERROR) break;

        error_previous(parser, parser->current.text);
    }
}

static bool match(Parser *parser, TokenType type) {
    if (parser->current.type != type) return false;
    advance(parser);
    return true;
}

static void consume(Parser *parser, TokenType type, StringView message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    error_previous(parser, message);
}

static bool check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static bool check_vartype(Parser *parser) {
    switch (parser->current.type) {
    case TOK_INT:
    case TOK_STR:
    case TOK_BOOL:
        return true;
    default:
        return false;
    }
}

static void synchronize(Parser *parser) {
    parser->panic_mode = false;

    while (parser->current.type != TOK_EOF) {
        advance(parser);
        if (parser->previous.type == TOK_SEMICOLON) return;
        switch (parser->current.type) {
            case TOK_IF:
            case TOK_LOOP:
            case TOK_FOR:
            case TOK_WHILE:
            case TOK_RETURN:
                return;
            default:
                ;
        }
    }
}

static bool sv_to_int(StringView s, int32_t *out, Arena *arena) {
    char *dup = sv_dup(arena, s);

    errno = 0;
    int64_t n = strtoll(dup, NULL, 0);
    if (errno == ERANGE)
        return false;
    
    if (n > 2147483647 || n < (-2147483647 - 1))
        return false;

    *out = (int32_t)n;
    return true;
}

static Expr *expression(Parser *parser);

static Expr *primary(Parser *parser) {
    if (match(parser, TOK_NUMBER)) {
        int32_t number;
        if (!sv_to_int(parser->previous.text, &number, parser->arena))
            error_previous(parser, SV("Value cannot be stored in a 32-bit integer."));

        ExprIntLit *expr;
        ALLOC_NODE(expr, parser->previous, ExprIntLit, 
            (ExprIntLit){ .value = number });
        return (Expr *)expr;
    }

    if (match(parser, TOK_STRING)) {
        ExprStrLit *expr;
        ALLOC_NODE(expr, parser->previous, ExprStrLit, 
            (ExprStrLit){ .value = parser->previous.text });
        return (Expr *)expr;
    }

    if (match(parser, TOK_TRUE) || match(parser, TOK_FALSE)) {
        ExprBoolLit *expr;
        ALLOC_NODE(expr, parser->previous, ExprBoolLit, 
            (ExprBoolLit){ .value = parser->previous.type == TOK_TRUE });
        return (Expr *)expr;
    }

    if (match(parser, TOK_IDENTIFIER)) {
        // Variable.
        Token name = parser->previous;
        ExprVar *expr;
        ALLOC_NODE(expr, name, ExprVar, (ExprVar){ .name = name.text });
        return (Expr *)expr;
    }

    if (match(parser, TOK_LEFT_PAREN)) {
        Expr *expr = expression(parser);
        consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after expression."));
        return expr;
    }

    error_current(parser, SV("Invalid expression."));
    return NULL;
}

static Expr *call(Parser *parser) {
    Expr *expr = primary(parser);

    while (true) {
        if (match(parser, TOK_LEFT_PAREN)) {
            VEC_PTR_Expr args;
            VEC_INIT(args);
            if (!check(parser, TOK_RIGHT_PAREN)) do {
                VEC_PUSH(args, expression(parser), parser->arena);
            } while (match(parser, TOK_COMMA));
    
            consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after argument list."));
    
            ExprCall *e;
            ALLOC_NODE(e, expr->loc, ExprCall,
                (ExprCall){ .callee = expr, .args = args });
        
            expr = (Expr *)e;
        } else if (match(parser, TOK_DOT)) {
            consume(parser, TOK_IDENTIFIER, SV("Expected field name."));

            ExprAccess *e;
            ALLOC_NODE(e, expr->loc, ExprAccess,
                (ExprAccess){ .left = expr, .name = parser->previous });

            expr = (Expr *)e;
        } else if (match(parser, TOK_LEFT_BRACK)) {
            Expr *index = expression(parser);
            consume(parser, TOK_RIGHT_BRACK, SV("Expected ']' after array index."));

            ExprArray *e;
            ALLOC_NODE(e, expr->loc, ExprArray,
                (ExprArray){ .left = expr, .index = index });
            
            expr = (Expr *)e;
        } else break;
    }

    return expr;
}

static Expr *unary(Parser *parser) {
    if (match(parser, TOK_BANG) ||
        match(parser, TOK_MINUS) ||
        match(parser, TOK_AMP)) {
        Token op = parser->previous;
        Expr *right = unary(parser);

        ExprUnary *expr;
        ALLOC_NODE(expr, op, ExprUnary, 
            (ExprUnary){ .right = right, .op = op });
        return (Expr *)expr;
    }

    return call(parser);    
}

static Expr *mul_div(Parser *parser) {
    Expr *expr = unary(parser);
    while (match(parser, TOK_STAR) ||
           match(parser, TOK_SLASH) ||
           match(parser, TOK_PERCENT)) {
        Token op = parser->previous;
        Expr *right = unary(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });

        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *add_sub(Parser *parser) {
    Expr *expr = mul_div(parser);
    while (match(parser, TOK_PLUS) ||
           match(parser, TOK_MINUS)) {
        Token op = parser->previous;
        Expr *right = mul_div(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *bit_shift(Parser *parser) {
    Expr *expr = add_sub(parser);
    while (match(parser, TOK_LESS_LESS) ||
           match(parser, TOK_GREATER_GREATER)) {
        Token op = parser->previous;
        Expr *right = add_sub(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *comparison(Parser *parser) {
    Expr *expr = bit_shift(parser);
    while (match(parser, TOK_GREATER) ||
           match(parser, TOK_GREATER_EQUAL) ||
           match(parser, TOK_LESS) ||
           match(parser, TOK_LESS_EQUAL)) {
        Token op = parser->previous;
        Expr *right = bit_shift(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *equality(Parser *parser) {
    Expr *expr = comparison(parser);
    while (match(parser, TOK_EQUAL_EQUAL) ||
           match(parser, TOK_BANG_EQUAL)) {
        Token op = parser->previous;
        Expr *right = comparison(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *bit_and(Parser *parser) {
    Expr *expr = equality(parser);
    while (match(parser, TOK_AMP)) {
        Token op = parser->previous;
        Expr *right = equality(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *bit_xor(Parser *parser) {
    Expr *expr = bit_and(parser);
    while (match(parser, TOK_CARET)) {
        Token op = parser->previous;
        Expr *right = bit_and(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *bit_or(Parser *parser) {
    Expr *expr = bit_xor(parser);
    while (match(parser, TOK_PIPE)) {
        Token op = parser->previous;
        Expr *right = bit_xor(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *and(Parser *parser) {
    Expr *expr = bit_or(parser);
    while (match(parser, TOK_AMP_AMP)) {
        Token op = parser->previous;
        Expr *right = bit_or(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *or(Parser *parser) {
    Expr *expr = and(parser);
    while (match(parser, TOK_PIPE_PIPE)) {
        Token op = parser->previous;
        Expr *right = and(parser);

        ExprBinary *binary_expr;
        ALLOC_NODE(binary_expr, op, ExprBinary, 
            (ExprBinary){ .left = expr, .right = right, .op = op });
    
        expr = (Expr *)binary_expr;
    }
    return expr;
}

static Expr *expression(Parser *parser) {
    return or(parser);
}

static Import import(Parser *parser) {
    consume(parser, TOK_STRING, SV("Expected file path."));

    StringView alias = SV_NULL;

    if (match(parser, TOK_AS)) {
        consume(parser, TOK_IDENTIFIER, SV("Expected an alias."));
        alias = parser->previous.text;
    }

    Import imp = (Import){
        .loc = parser->previous,
        .path = parser->previous.text,
        .alias = alias
    };

    consume(parser, TOK_SEMICOLON, SV("Expected ';' after import."));

    return imp;
}

static Stmt *statement(Parser *parser);

static Stmt *return_stmt(Parser *parser) {
    Expr *expr = NULL;
    if (!check(parser, TOK_SEMICOLON))
        expr = expression(parser);
    
    StmtReturn *stmt;
    ALLOC_NODE(stmt, parser->previous, StmtReturn,
        (StmtReturn){ .expr = expr });

    consume(parser, TOK_SEMICOLON, SV("Expected ';' after 'return'."));
    return (Stmt *)stmt;
}

static Stmt *continue_stmt(Parser *parser) {
    StmtContinue *stmt;
    ALLOC_NODE(stmt, parser->previous, StmtContinue,
        (StmtContinue){0});

    consume(parser, TOK_SEMICOLON, SV("Expected ';' after 'continue'."));
    return (Stmt *)stmt;
}

static Stmt *break_stmt(Parser *parser) {
    StmtBreak *stmt;
    ALLOC_NODE(stmt, parser->previous, StmtBreak,
        (StmtBreak){0});

    consume(parser, TOK_SEMICOLON, SV("Expected ';' after 'break'."));
    return (Stmt *)stmt;
}

static Stmt *var_decl(Parser *parser, bool parse_const, bool parse_initializer) {
    bool is_const = false;
    
    if (parse_const && match(parser, TOK_CONST)) {
        is_const = true;
        advance(parser);
    }

    if (!match(parser, TOK_INT) && !match(parser, TOK_STR) && !match(parser, TOK_BOOL))
        error_current(parser, SV("Invalid declaration type."));

    Token var_type = parser->previous;

    Expr *array_length = NULL;
    if (match(parser, TOK_LEFT_BRACK)) {
        array_length = expression(parser);
        consume(parser, TOK_RIGHT_BRACK, SV("Expected ']' after array length."));
    }

    consume(parser, TOK_IDENTIFIER, SV("Expected variable name."));
    StringView param_name = parser->previous.text;

    Expr *initializer = NULL;
    if (parse_initializer) {
        if (match(parser, TOK_EQUAL)) {
            initializer = expression(parser);
        }
    }

    StmtVarDecl *stmt;
    ALLOC_NODE(stmt, parser->previous, StmtVarDecl, 
        (StmtVarDecl){ .type = var_type, .name = param_name,
            .array_length = array_length, .initializer = initializer,
            .is_const = is_const });

    return (Stmt *)stmt;
}

static Stmt *if_stmt(Parser *parser) {
    Token tok = parser->previous;

    consume(parser, TOK_LEFT_PAREN, SV("Expected '(' after 'if'."));
    Expr *condition = expression(parser);
    consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after 'if' condition."));

    Stmt *then_branch = statement(parser);

    Stmt *else_branch = NULL;
    if (match(parser, TOK_ELSE))
        else_branch = statement(parser);

    StmtIf* stmt;
    ALLOC_NODE(stmt, tok, StmtIf, 
        (StmtIf){ .condition = condition,
            .then_branch = then_branch, .else_branch = else_branch });
    
    return (Stmt *)stmt;
}

static Stmt *loop_stmt(Parser *parser) {
    Token tok = parser->previous;
    
    Expr *count = NULL;
    if (match(parser, TOK_LEFT_PAREN)) {
        count = expression(parser);
        consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after loop count."));
    }

    Stmt *body = statement(parser);

    StmtLoop *stmt;
    ALLOC_NODE(stmt, tok, StmtLoop,
        (StmtLoop){ .count = count, .body = body });

    return (Stmt *)stmt;
}

static Stmt *while_stmt(Parser *parser) {
    Token tok = parser->previous;

    consume(parser, TOK_LEFT_PAREN, SV("Expected '(' after 'while'."));
    Expr *condition = expression(parser);
    consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after 'while' condition."));

    Stmt *body = statement(parser);

    StmtIf *if_wrapper;
    ALLOC_NODE(if_wrapper, tok, StmtIf,
        (StmtIf){ .condition = condition, .then_branch = body,
            .else_branch = break_stmt(parser) });

    StmtLoop *stmt;
    ALLOC_NODE(stmt, tok, StmtLoop,
        (StmtLoop){ .count = NULL, .body = (Stmt *)if_wrapper });
    
    return (Stmt *)stmt;
}

static Stmt *for_stmt(Parser *parser) {
    Token tok = parser->previous;

    consume(parser, TOK_LEFT_PAREN, SV("Expected '(' after 'for'."));
    
    consume(parser, TOK_INT, SV("Expected 'int'."));
    consume(parser, TOK_IDENTIFIER, SV("Expected variable name."));
    Token ident = parser->previous;
    
    consume(parser, TOK_IN, SV("Expected 'in' after declaration."));
    
    Expr *left_bound = expression(parser);
    consume(parser, TOK_DOT_DOT, SV("Expected '..' after left bound."));
    Expr *right_bound = expression(parser);

    consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after 'for' condition."));

    Stmt *body = statement(parser);

    StmtFor *stmt;
    ALLOC_NODE(stmt, tok, StmtFor,
        (StmtFor){ .iterator = ident.text,
            .left_bound = left_bound, .right_bound = right_bound, .body = body });
    
    return (Stmt *)stmt;
}

static Stmt *cmd_stmt(Parser *parser) {
    Token tok = parser->previous;
    
    consume(parser, TOK_LEFT_BRACK, SV("Expected '[' after 'cmd'."));
    Expr* cmd_id = expression(parser);
    consume(parser, TOK_RIGHT_BRACK, SV("Expected ']' after 'cmd' ID."));

    consume(parser, TOK_LEFT_PAREN, SV("Expected '(' before 'cmd' integer operands."));
    VEC_PTR_Expr int_operands;
    VEC_INIT(int_operands);
    if (!check(parser, TOK_RIGHT_PAREN)) do {
        VEC_PUSH(int_operands, expression(parser), parser->arena);
    } while (match(parser, TOK_COMMA));
    consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after 'cmd' integer operands."));

    consume(parser, TOK_LEFT_PAREN, SV("Expected '(' before 'cmd' string operands."));
    VEC_PTR_Expr str_operands;
    VEC_INIT(str_operands);
    if (!check(parser, TOK_RIGHT_PAREN)) do {
        VEC_PUSH(str_operands, expression(parser), parser->arena);
    } while (match(parser, TOK_COMMA));
    consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after 'cmd' string operands."));
    
    consume(parser, TOK_SEMICOLON, SV("Expected ';' after statement."));


    StmtCmd *stmt;
    ALLOC_NODE(stmt, tok, StmtCmd, 
        (StmtCmd){ .id = cmd_id,
            .int_operands = int_operands, .str_operands = str_operands });

    return (Stmt *)stmt;
}

static Stmt *assign_stmt(Parser *parser) {
    Expr *lhs = expression(parser);
    if (lhs && lhs->kind == NODE_ExprCall &&
        match(parser, TOK_SEMICOLON)) {

        StmtExpr *stmt;
        ALLOC_NODE(stmt, lhs->loc, StmtExpr,
            (StmtExpr){ .expr = lhs });
        return (Stmt *)stmt;
    }

    consume(parser, TOK_EQUAL, SV("Expected '='."));
    Token tok = parser->previous;
    Expr *rhs = expression(parser);
    consume(parser, TOK_SEMICOLON, SV("Expected ';' after assignment."));

    StmtAssign *stmt;
    ALLOC_NODE(stmt, tok, StmtAssign,
        (StmtAssign){ .left = lhs, .assign_type = tok, .right = rhs });

    return (Stmt *)stmt;
}

static Stmt *declaration(Parser *parser) {
    if (parser->panic_mode) synchronize(parser);
    if (check_vartype(parser) || 
        check(parser, TOK_CONST)) {

        Stmt *stmt = var_decl(parser, true, true);
        consume(parser, TOK_SEMICOLON, SV("Expected ';' after declaration."));
        return stmt;
    }

    return statement(parser);
}

static VEC_PTR_Stmt block(Parser *parser) {
    VEC_PTR_Stmt stmts;
    VEC_INIT(stmts);
    while (!check(parser, TOK_RIGHT_BRACE) && !check(parser, TOK_EOF))
        VEC_PUSH(stmts, declaration(parser), parser->arena);

    consume(parser, TOK_RIGHT_BRACE, SV("Expected '}' after block."));
    return stmts;
}

static Stmt *statement(Parser *parser) {
    if (match(parser, TOK_IMPORT))
        error_previous(parser, SV("Can only import at beginning of file."));
    
    if (match(parser, TOK_IF)) return if_stmt(parser);
    if (match(parser, TOK_LOOP)) return loop_stmt(parser);
    if (match(parser, TOK_WHILE)) return while_stmt(parser);
    if (match(parser, TOK_FOR)) return for_stmt(parser);
    if (match(parser, TOK_RETURN)) return return_stmt(parser);
    if (match(parser, TOK_CONTINUE)) return continue_stmt(parser);
    if (match(parser, TOK_BREAK)) return break_stmt(parser);
    if (match(parser, TOK_CMD)) return cmd_stmt(parser);
    if (match(parser, TOK_LEFT_BRACE)) {
        VEC_PTR_Stmt stmts = block(parser);
        StmtBlock *stmt;
        ALLOC_NODE(stmt, parser->previous, StmtBlock,
            (StmtBlock){ .stmts = stmts });
        return (Stmt *)stmt;
    }

    return assign_stmt(parser);
}

static Stmt *function_decl(Parser *parser) {
    bool is_inline = false;
    if (parser->previous.type == TOK_INLINE) {
        is_inline = true;
        advance(parser);
    }

    switch (parser->previous.type) {
        case TOK_VOID:
        case TOK_INT:
        case TOK_STR:
        case TOK_BOOL:
            break;
        default:
            error_current(parser, SV("Unexpected return type."));
    }
    Token ret = parser->previous;

    consume(parser, TOK_IDENTIFIER, SV("Expected function name."));
    Token loc = parser->previous;

    consume(parser, TOK_LEFT_PAREN, SV("Expected '(' after function name."));
    
    VEC_PTR_StmtVarDecl params;
    VEC_INIT(params);
    if (!check(parser, TOK_RIGHT_PAREN)) do {
        StmtVarDecl *stmt = (StmtVarDecl *)var_decl(parser, false, false);
        VEC_PUSH(params, stmt, parser->arena);
    } while (match(parser, TOK_COMMA));

    consume(parser, TOK_RIGHT_PAREN, SV("Expected ')' after parameter list."));
    consume(parser, TOK_LEFT_BRACE, SV("Expected '{' before function body."));

    VEC_PTR_Stmt body = block(parser);

    StmtFuncDecl *stmt;
    ALLOC_NODE(stmt, loc, StmtFuncDecl,
        (StmtFuncDecl){ .ret = ret, .name = loc.text, .params = params,
            .body = body, .is_inline = is_inline });

    return (Stmt *)stmt;
}

static Stmt *db_decl(Parser *parser) {
    Token db = parser->previous;

    consume(parser, TOK_IDENTIFIER, SV("Expected DB name."));
    StringView name = parser->previous.text;

    consume(parser, TOK_LEFT_BRACE, SV("Expected '{' before DB fields."));

    VEC_PTR_StmtVarDecl fields;
    VEC_INIT(fields);
    while (check_vartype(parser)) {
        StmtVarDecl *stmt = (StmtVarDecl *)var_decl(parser, false, false);
        consume(parser, TOK_SEMICOLON, SV("Expected ';' after declaration."));
        VEC_PUSH(fields, stmt, parser->arena);
    }

    consume(parser, TOK_RIGHT_BRACE, SV("Expected '}' after DB fields."));

    StmtDBDecl *stmt;
    ALLOC_NODE(stmt, db, StmtDBDecl,
        (StmtDBDecl){ .name = name,
            .db = db, .fields = fields });
   
    return (Stmt *)stmt;
}

static Stmt *top_decl(Parser *parser) {
    if (match(parser, TOK_CDB) ||
        match(parser, TOK_UDB)) {
        return db_decl(parser);
    }

    if (match(parser, TOK_INLINE) ||
        match(parser, TOK_VOID) ||
        match(parser, TOK_INT) ||
        match(parser, TOK_STR)) {

        return function_decl(parser);
    }

    if (match(parser, TOK_CONST)) {
        return var_decl(parser, true, true);
    }

    if (match(parser, TOK_IMPORT)) {
        error_previous(parser, SV("Imports must go at the top of a file."));
        return NULL;
    }
    
    error_current(parser, SV("Unexpected declaration."));
    return NULL;
}

ProgramAST *generate_ast(StringView file_path, const char *source, Arena *arena) {
    Parser parser;
    lexer_init(&parser.lexer, source);

    parser.panic_mode = false;
    parser.had_error = false;

    parser.arena = arena;
    parser.source = source;
    parser.file_path = file_path;

    ProgramAST *ast = arena_alloc(arena, sizeof(ProgramAST));
    if (!ast) {
        fprintf(stderr, "Could not allocate AST.");
        return NULL;
    }
    ast->file = file_path;
    ast->source = source;
    VEC_INIT(ast->imports);
    VEC_INIT(ast->stmts);
    
    advance(&parser);

    while (match(&parser, TOK_IMPORT)) {
        VEC_PUSH(ast->imports, import(&parser), arena);
        if (parser.panic_mode) synchronize(&parser);
    }

    while (!match(&parser, TOK_EOF)) {
        VEC_PUSH(ast->stmts, top_decl(&parser), arena);
        if (parser.panic_mode) synchronize(&parser);
    }

    return parser.had_error ? NULL : ast;
}