#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "error.h"

#define ALLOC_NODE(var, token, type_, ...) \
    do { \
        (var) = arena_alloc(parser->arena, sizeof(type_)); \
        if (!(var)) { \
            fprintf(stderr, "Fatal error: Out of memory."); \
            exit(1); \
        } \
        *var = __VA_ARGS__; \
        (var)->base.loc = (token); \
        (var)->base.type = NODE_##type_; \
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

static void synchronize(Parser *parser) {
    parser->panic_mode = false;

    while (parser->current.type != TOK_EOF) {
        if (parser->previous.type == TOK_SEMICOLON) return;
        switch (parser->current.type) {
            case TOK_IF:
            case TOK_LOOP:
            case TOK_FOR:
            case TOK_WHILE:
            case TOK_RETURN:
            case TOK_SM:
                return;
            default:
                ;
        }
        advance(parser);
    }
}

static bool sv_to_int(StringView s, int32_t *out) {

    // Hexadecimal.
    if (s.data[0] == '0' &&
        (s.data[1] == 'x' || s.data[1] == 'X')) {

        int32_t total = 0;
        int32_t multiplier = 1;
        for (size_t i = s.len - 1; i >= 2; i--) {
            int32_t digit;
            if (s.data[i] >= 'A' && s.data[i] <= 'F')
                digit = s.data[i] - 'A' + 10;
            else if (s.data[i] >= 'a' && s.data[i] <= 'f')
                digit = s.data[i] - 'a' + 10;
            else
                digit = s.data[i] - '0';
            
            total += digit * multiplier;
            
            multiplier *= 16;
        }

        *out = total;
        return true;
    }

    // Binary.
    if (s.data[0] == '0' &&
        (s.data[1] == 'b' || s.data[1] == 'B')) {

        int32_t total = 0;
        int32_t multiplier = 1;
        for (size_t i = s.len - 1; i >= 2; i--) {
            int32_t digit = s.data[i] - '0';
            
            total += digit * multiplier;
            
            multiplier *= 2;
        }

        *out = total;
        return true;
    }

    // Decimal.
    {
        int32_t total = 0;
        int32_t multiplier = 1;
        for (size_t i = s.len - 1; i != -1; i--) {
            int32_t digit = s.data[i] - '0';
            
            total += digit * multiplier;
            if (total < 0) return false;
            
            multiplier *= 10;
        }

        *out = total;
        return true;
    }
}

static Expr *expression(Parser *parser);

static Expr *primary(Parser *parser) {
    if (match(parser, TOK_NUMBER)) {
        int32_t number;
        if (!sv_to_int(parser->previous.text, &number))
            error_previous(parser, SV("Value cannot be stored in 32-bit integer."));

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

static Stmt *import(Parser *parser) {
    consume(parser, TOK_STRING, SV("Expected file path."));
    
    StmtImport *stmt;
    ALLOC_NODE(stmt, parser->previous, StmtImport,
        (StmtImport){ .path = parser->previous.text });

    consume(parser, TOK_SEMICOLON, SV("Expected ';' after import."));

    return (Stmt *)stmt;
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

static Stmt *var_decl(Parser *parser, bool parse_initializer) {
    bool is_const = false;
    if (match(parser, TOK_CONST)) {
        is_const = true;
        advance(parser);
    }

    if (!match(parser, TOK_INT) && !match(parser, TOK_STR))
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
            .array_length = array_length, .initializer = initializer });

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
    if (lhs && lhs->type == NODE_ExprCall &&
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
    if (check(parser, TOK_INT) || 
        check(parser, TOK_STR) ||
        check(parser, TOK_CONST)) {

        Stmt *stmt = var_decl(parser, true);
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
        StmtVarDecl *stmt = (StmtVarDecl *)var_decl(parser, false);
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

static VEC_PTR_Stmt sm_body(Parser *parser) {
    VEC_PTR_Stmt stmts;
    VEC_INIT(stmts);

    // Declarations.
    while (match(parser, TOK_STATE) || match(parser, TOK_DECL)) {
        bool has_state = false;
        if (parser->previous.type == TOK_STATE) {
            consume(parser, TOK_DECL, SV("Expected 'decl' after 'state'."));
            has_state = true;
        }
        
        consume(parser, TOK_LEFT_BRACE, SV("Expected '{' before decl type."));
        
        Token decl_type = parser->current;
        if (!match(parser, TOK_ANY) &&
            !match(parser, TOK_ANY_CALL) &&
            !match(parser, TOK_ANY_ARGS)) {
            
            error_current(parser, SV("Unexpected decl type."));
        }
        decl_type = parser->previous;

        consume(parser, TOK_RIGHT_BRACE, SV("Expected '}' after decl type."));

        consume(parser, TOK_IDENTIFIER, SV("Expected decl name."));
        Token loc = parser->previous;

        consume(parser, TOK_SEMICOLON, SV("Expected ';' after decl."));
        
        StmtVarDecl *stmt;
        ALLOC_NODE(stmt, loc, StmtVarDecl,
            (StmtVarDecl){ .type = decl_type, .name = loc.text,
                .smvar_has_state = has_state });

        VEC_PUSH(stmts, (Stmt *)stmt, parser->arena);
    }

    // States.
    while (match(parser, TOK_IDENTIFIER)) {
        Token state_loc = parser->previous;
        consume(parser, TOK_COLON, SV("Expected colon after state name."));

        VEC_PTR_ExprSMMatch matches;
        VEC_INIT(matches);

        do {
            Expr *to_match = NULL;
            Token loc;
            if (match(parser, TOK_END_OF_PATH)) {
                ALLOC_NODE((ExprSMEndOfPath *)to_match,
                    parser->previous, ExprSMEndOfPath, (ExprSMEndOfPath){0});
            } else {
                consume(parser, TOK_LEFT_BRACE, SV("Expected '{' before state rule."));
                loc = parser->previous;
                to_match = expression(parser);
    
                consume(parser, TOK_RIGHT_BRACE, SV("Expected '}' after pattern."));
            }
            consume(parser, TOK_SM_ARROW, SV("Expected '==>' after pattern."));
            
            ExprSMMatch *expr;
            if (match(parser, TOK_LEFT_BRACE)) {
                VEC_PTR_Stmt action = block(parser);

                ALLOC_NODE(expr, loc, ExprSMMatch,
                    (ExprSMMatch){ .expr_pattern = to_match,
                        .next_state = SV(""), .action = action, });
            } else {
                consume(parser, TOK_IDENTIFIER, SV("Expected state name."));
                
                ALLOC_NODE(expr, loc, ExprSMMatch,
                    (ExprSMMatch){ .expr_pattern = to_match,
                        .next_state = parser->previous.text, });
            }
            VEC_PUSH(matches, expr, parser->arena);   
        
        } while (match(parser, TOK_PIPE));

        consume(parser, TOK_SEMICOLON, SV("Expected ';' after state rule."));


        StmtSMState *stmt;
        ALLOC_NODE(stmt, state_loc, StmtSMState,
            (StmtSMState){ .name = state_loc.text, .matches = matches, });
        
        VEC_PUSH(stmts, (Stmt *)stmt, parser->arena);
    }

    consume(parser, TOK_RIGHT_BRACE, SV("Expected '}' after SM body."));

    return stmts;
}

static Stmt *sm_decl(Parser *parser) {
    bool flow_insensitive = false;
    if (match(parser, TOK_FLOW_INSENSITIVE))
        flow_insensitive = true;

    consume(parser, TOK_IDENTIFIER, SV("Expected SM name."));
    Token loc = parser->previous;

    consume(parser, TOK_LEFT_BRACE, SV("Expected '{' before SM body."));
    VEC_PTR_Stmt body = sm_body(parser);

    StmtSMDecl *stmt;
    ALLOC_NODE(stmt, loc, StmtSMDecl,
        (StmtSMDecl){ .name = loc.text,
            .flow_insensitive = flow_insensitive, .body = body,});
    
    return (Stmt *)stmt;
}

static Stmt *top_decl(Parser *parser) {
    if (match(parser, TOK_SM)) {
        return sm_decl(parser);
    }

    if (match(parser, TOK_INLINE) ||
        match(parser, TOK_VOID) ||
        match(parser, TOK_INT) ||
        match(parser, TOK_STR)) {

        return function_decl(parser);
    }

    if (match(parser, TOK_CONST)) {
        return var_decl(parser, true);
    }
    
    error_current(parser, SV("Unexpected declaration."));
    return NULL;
}

VEC_PTR_Stmt *generate_ast(StringView file_path, const char *source, Arena *arena) {
    Parser parser;
    lexer_init(&parser.lexer, source);

    parser.panic_mode = false;
    parser.had_error = false;

    parser.arena = arena;
    parser.source = source;
    parser.file_path = file_path;

    VEC_PTR_Stmt *stmts = arena_alloc(arena, sizeof(VEC_PTR_Stmt));
    if (!stmts) {
        fprintf(stderr, "Could not allocate AST.");
        return NULL;
    }
    VEC_INIT(*stmts);
    
    advance(&parser);

    while (match(&parser, TOK_IMPORT)) {
        VEC_PUSH(*stmts, import(&parser), arena);
        if (parser.panic_mode) synchronize(&parser);
    }

    while (!match(&parser, TOK_EOF)) {
        VEC_PUSH(*stmts, top_decl(&parser), arena);
        if (parser.panic_mode) synchronize(&parser);
    }

    return parser.had_error ? NULL : stmts;
}