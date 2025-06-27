#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include "token.h"
#include "ast.h"

class Parser {
public:
    Parser(std::vector<Token> tokens) : tokens(tokens) {};

    std::vector<Stmt*> parse() {
        while (!is_at_end()) {
            statements.push_back(global_stmt());
        }
        return statements;
    }

    bool failed() { return had_error; }

private:
    std::vector<Token> tokens;
    std::vector<Stmt*> statements;
    size_t pos = 0;
    bool had_error = false;

    // statements
    Stmt* global_stmt() {
        try {
            if (match({T_INLINE, T_VOID, T_INT, T_STR})) return function_decl();
            if (match(T_CONST)) return var_decl();
            if (match(T_CDB)) return cdb_stmt();
            // if (match(T_TYPEDEF)) return typedef_stmt();
            error(peek(), "Invalid statement");
        } catch (std::runtime_error e) {
            synchronize();
            return nullptr;
        }
        return nullptr;
    }

    Stmt* cdb_stmt() {
        Token* tok = eat(T_IDENT, "Expected a name after 'cdb'");
        eat(T_LBRACE, "Expected '{' after cdb name");

        std::vector<VarStmt*> fields;
        while (match({T_INT, T_STR})) {
            WodType field_type;
            switch(previous()->token_type) {
                case T_INT: field_type = WodType(TYPE_INT); break;
                case T_STR: field_type = WodType(TYPE_STR); break;
                default:
                    error(previous(), "Unexpected field type");
                    break;
            }

            Expr* arr_len = nullptr;
            if (match(T_LBRACK)) {
                field_type = WodType(TYPE_INTARR);
                arr_len = expression();
                eat(T_RBRACK, "Expected ']' after array length");
            }

            Token* ntok = eat(T_IDENT, "Expected field name");
            
            Expr* initializer = nullptr;
            if (match(T_EQUAL)) {
                initializer = expression();
                error(ntok, "CDB field initializers are not yet supported");
            }
            fields.push_back(new VarStmt(ntok,
                false, field_type, arr_len, ntok->text, initializer));
            eat(T_SEMICOLON, "Expected ';' after cdb field");
        }
        eat(T_RBRACE, "Expected '}' at end of cdb declaration");

        return new CdbStmt(tok, tok->text, fields);
    }

    Stmt* function_decl() {
        bool is_inline = false;
        if (previous()->token_type == T_INLINE) {
            is_inline = true;
            advance();
        }

        WodType type;
        switch (previous()->token_type) {
            case T_VOID: type = WodType(TYPE_VOID); break;
            case T_INT:  type = WodType(TYPE_INT); break;
            case T_STR:  type = WodType(TYPE_STR); break;
            default: error(previous(), "Invalid function return type"); break;
        }
        Token* tok = eat(T_IDENT, "Expected function name");
        eat(T_LPAREN, "Expected '(' after function name");
        std::vector<VarStmt*> params;
        if (!check(T_RPAREN)) do {
            if (match({T_INT, T_STR})) {
                WodType param_type;
                switch(previous()->token_type) {
                    case T_INT: param_type = TYPE_INT; break;
                    case T_STR: param_type = TYPE_STR; break;
                }
                Expr* arr_length = nullptr;
                if (match(T_LBRACK)) {
                    arr_length = expression();
                    eat(T_RBRACK, "Expected ']' after array length");
                    param_type = WodType(TYPE_INTARR);
                }
                Token* ntok = eat(T_IDENT, "Expected parameter name");
                params.push_back(new VarStmt{ntok, false, param_type, arr_length, ntok->text, nullptr});
            } else {
                error(peek(), "Invalid parameter type");
            }
        } while (match(T_COMMA));

        eat(T_RPAREN, "Expected ')' after function parameters");
        eat(T_LBRACE, "Expected '{' before function body");
        return new FunctionStmt(tok, type, tok->text, params, block(), is_inline);
    }

    std::vector<Stmt*> block() {
        std::vector<Stmt*> block_stmts;
        while (!check(T_RBRACE) && !is_at_end()) {
            block_stmts.push_back(statement());
        }
        eat(T_RBRACE, "Expected '}' after block");
        return block_stmts;
    }

    
    Stmt* statement() {
        if (match(T_IF)) return if_stmt();
        if (match(T_LOOP)) return loop_stmt();
        if (match(T_WHILE)) return while_stmt();
        if (match(T_FOR)) return for_stmt();
        if (match(T_RETURN)) return return_stmt();
        if (match(T_CONTINUE)) {
            eat(T_SEMICOLON, "Expected ';' after 'continue'");
            return new ContinueStmt(previous());
        }
        if (match(T_BREAK)) {
            eat(T_SEMICOLON, "Expected ';' after 'break'");
            return new BreakStmt(previous());
        }
        if (match({T_INT, T_STR, T_CONST})) return var_decl();
        if (match(T_CMD)) return cmd_stmt();
        if (match(T_LBRACE)) return new BlockStmt(previous(), block());
        // if (match(T_TYPEDEF)) return typedef_stmt();

        return expr_stmt();
    }

    // Stmt* typedef_stmt() {
    //     if (!match({T_VOID, T_INT, T_STR})) {
    //         error(previous(), "Unexpected typedef");
    //     }
    //     Token* tok = previous();
    //     WodType ty;
    //     switch (tok->token_type) {
    //         case T_VOID:    ty = WodType(TYPE_VOID); break;
    //         case T_INT:     ty = WodType(TYPE_INT); break;
    //         case T_STR:     ty = WodType(TYPE_STR); break;
    //     }
        
    //     Expr* expr = nullptr;
    //     if (match(T_LBRACK)) {
    //         ty = WodType(TYPE_INTARR);
    //         expr = expression();
    //         eat(T_RBRACK, "Expected ']' after array length");
    //     }
    //     Token* n_tok = eat(T_IDENT, "Expected identifier after type");
    //     eat(T_SEMICOLON, "Expected ';' after typedef declaration");

    //     return new TypedefStmt(tok, ty, expr, n_tok->text);
    // }

    Stmt* while_stmt() {
        Token* tok = previous();
        eat(T_LPAREN, "Expected '(' after 'while'");
        Expr* condition = expression();
        eat(T_RPAREN, "Expected ')' after while condition");
        Stmt* body = statement();
        Stmt* if_wrapper = new IfStmt(tok, condition, body, new BreakStmt(tok));
        return new LoopStmt(tok, nullptr, if_wrapper);
    }

    Stmt* for_stmt() {
        Token* tok = previous();
        eat(T_LPAREN, "Expected '(' after 'for'");

        Stmt* initializer;
        if (match(T_SEMICOLON)) initializer = nullptr;
        else if (match({T_INT, T_STR})) initializer = var_decl();
        else initializer = expr_stmt();

        Expr* condition = nullptr;
        if (!check(T_SEMICOLON)) {
            condition = expression();
        }
        eat(T_SEMICOLON, "Expected ';' after loop condition");
        
        Expr* increment = nullptr;
        if (!check(T_RPAREN)) {
            increment = expression();
        }
        eat(T_RPAREN, "Expected ')' after for clause");
        Stmt* body = statement();
        
        if (increment) body = new BlockStmt(tok, {body, new ExprStmt(tok, increment)});            
        if (condition) body = new IfStmt(tok, condition, body, new BreakStmt(tok));
        body = new LoopStmt(tok, nullptr, body);
        
        if (initializer) body = new BlockStmt(tok, {initializer, body});

        return body;
    }

    Stmt* cmd_stmt() {
        Token* tok = previous();

        eat(T_LBRACK, "Expected '[' after 'cmd'");
        Expr* cmd_id = expression();
        eat(T_RBRACK, "Expected ']' after cmd id");

        eat(T_LPAREN, "Expected '(' before cmd integer arguments");
        std::vector<Expr*> int_fields;
        if (!check(T_RPAREN)) do {
            int_fields.push_back(expression());
        } while (match(T_COMMA));
        eat(T_RPAREN, "Expected ')' after cmd integer arguments");
        
        eat(T_LPAREN, "Expected '(' before cmd string arguments");
        std::vector<Expr*> str_fields;
        if (!check(T_RPAREN)) do {
            str_fields.push_back(expression());
        } while (match(T_COMMA));
        eat(T_RPAREN, "Expected ')' after cmd string arguments");
        eat(T_SEMICOLON, "Expected ';' after cmd statement");
        
        return new CmdStmt(tok, cmd_id, int_fields, str_fields);
    }

    Stmt* var_decl() {
        bool is_const = false;
        if (previous()->token_type == T_CONST) {
            is_const = true;
            advance();
        }
        WodType type;
        switch (previous()->token_type) {
            case T_INT: type = WodType(TYPE_INT); break;
            case T_STR: type = WodType(TYPE_STR); break;
        }
        Expr* arr_length = nullptr;
        if (match(T_LBRACK)) {
            arr_length = expression();
            eat(T_RBRACK, "Expected ']' after array length");
            type = WodType(TYPE_INTARR);
        }
        Token* tok = eat(T_IDENT, "Expected variable name");

        Expr* initializer = nullptr;
        if (!arr_length) { // currently no support for initializing arrays
            if (match(T_EQUAL)) initializer = expression();
        }
        eat(T_SEMICOLON, "Expected ';' after variable declaration");
        return new VarStmt(tok, is_const, type, arr_length, tok->text, initializer);
    }

    Stmt* if_stmt() {
        Token* tok = previous();
        eat(T_LPAREN, "Expected '(' after 'if'");
        Expr* condition = expression();
        eat(T_RPAREN, "Expected ')' after if condition");
        Stmt* then_branch = statement();
        Stmt* else_branch = nullptr;
        if (match(T_ELSE)) else_branch = statement();
        return new IfStmt(tok, condition, then_branch, else_branch);
    }

    Stmt* loop_stmt() {
        Token* tok = previous();
        Expr* count = nullptr;
        if (match(T_LPAREN)) {
            count = expression();
            eat(T_RPAREN, "Expected ')' after loop count");
        }
        Stmt* body = statement();
        return new LoopStmt(tok, count, body);
    }

    Stmt* return_stmt() {
        Token* tok = previous();
        Expr* expr = nullptr;
        if (!check(T_SEMICOLON)) expr = expression();
        eat(T_SEMICOLON, "Expected ';' after return statement");
        return new ReturnStmt(tok, expr);
    }

    Stmt* expr_stmt() {
        Token* tok = peek();
        Expr* expr = expression();
        eat(T_SEMICOLON, "Expected ';' after expression");
        return new ExprStmt(tok, expr);
    }

    // expressions
    Expr* expression() {
        return assignment();
    }

    Expr* assignment() {
        Expr* expr = or();
        if (match({T_EQUAL, T_PLUS_EQUAL, T_MINUS_EQUAL, T_STAR_EQUAL, T_SLASH_EQUAL, T_PERCENT_EQUAL})) {
            Token* tok = previous();
            Expr* value = or();
            AssignExpr::AssignOp op;
            switch (tok->token_type) {
                case T_EQUAL:           op = AssignExpr::EQUAL; break;
                case T_PLUS_EQUAL:      op = AssignExpr::PLUS_EQUAL; break;
                case T_MINUS_EQUAL:     op = AssignExpr::MINUS_EQUAL; break;
                case T_STAR_EQUAL:      op = AssignExpr::TIMES_EQUAL; break;
                case T_SLASH_EQUAL:     op = AssignExpr::DIV_EQUAL; break;
                case T_PERCENT_EQUAL:   op = AssignExpr::MOD_EQUAL; break;
            }
            return new AssignExpr(tok, expr, op, value);
        }
        return expr;
    }

    Expr* or() {
        Expr* expr = and();
        while (match(T_PIPE_PIPE)) {
            Token* tok = previous();
            Expr* right = and();
            expr = new BinaryExpr(tok, expr, BinaryExpr::LOGIC_OR, right);
        }
        return expr;
    }

    Expr* and() {
        Expr* expr = bit_or();
        while (match(T_AMP_AMP)) {
            Token* tok = previous();
            Expr* right = bit_or();
            expr = new BinaryExpr(tok, expr, BinaryExpr::LOGIC_AND, right);
        }
        return expr;
    }

    Expr* bit_or() {
        Expr* expr = bit_xor();
        while (match(T_PIPE)) {
            Token* tok = previous();
            Expr* right = bit_xor();
            expr = new BinaryExpr(tok, expr, BinaryExpr::BIT_OR, right);
        }
        return expr;
    }

    Expr* bit_xor() {
        Expr* expr = bit_and();
        while (match(T_CARET)) {
            Token* tok = previous();
            Expr* right = bit_and();
            expr = new BinaryExpr(tok, expr, BinaryExpr::BIT_XOR, right);
        }
        return expr;
    }

    Expr* bit_and() {
        Expr* expr = equality();
        while (match(T_AMP)) {
            Token* tok = previous();
            Expr* right = equality();
            expr = new BinaryExpr(tok, expr, BinaryExpr::BIT_AND, right);
        }
        return expr;
    }

    Expr* equality() {
        Expr* expr = comparison();
        while (match({T_EQUAL_EQUAL, T_BANG_EQUAL})) {
            Token* tok = previous();
            Expr* right = comparison();
            BinaryExpr::BinaryOp op;
            switch (tok->token_type) {
                case T_EQUAL_EQUAL: op = BinaryExpr::EQ; break;
                case T_BANG_EQUAL:  op = BinaryExpr::NEQ; break;
            }
            expr = new BinaryExpr(tok, expr, op, right);
        }
        return expr;
    }

    Expr* comparison() {
        Expr* expr = bit_shift();
        while (match({T_GREATER, T_GREATER_EQUAL, T_LESS, T_LESS_EQUAL})) {
            Token* tok = previous();
            Expr* right = bit_shift();
            BinaryExpr::BinaryOp op;
            switch (tok->token_type) {
                case T_GREATER:         op = BinaryExpr::GT; break;
                case T_GREATER_EQUAL:   op = BinaryExpr::GTE; break;
                case T_LESS:            op = BinaryExpr::LT; break;
                case T_LESS_EQUAL:      op = BinaryExpr::LTE; break;
            }
            expr = new BinaryExpr(tok, expr, op, right);
        }
        return expr;
    }

    Expr* bit_shift() {
        Expr* expr = addsub();
        while (match({T_LESS_LESS, T_GREATER_GREATER})) {
            Token* tok = previous();
            Expr* right = addsub();
            BinaryExpr::BinaryOp op;
            switch (tok->token_type) {
                case T_LESS_LESS:         op = BinaryExpr::LSHIFT; break;
                case T_GREATER_GREATER:   op = BinaryExpr::RSHIFT; break;
            }
            expr = new BinaryExpr(tok, expr, op, right);
        }
        return expr;
    }
    
    Expr* addsub() {
        Expr* expr = muldiv();
        while (match({T_PLUS, T_MINUS})) {
            Token* tok = previous();
            Expr* right = muldiv();
            BinaryExpr::BinaryOp op;
            switch (tok->token_type) {
                case T_PLUS:    op = BinaryExpr::ADD; break;
                case T_MINUS:   op = BinaryExpr::SUB; break;
            }
            expr = new BinaryExpr(tok, expr, op, right);
        }
        return expr;
    }

    Expr* muldiv() {
        Expr* expr = unary();
        while (match({T_STAR, T_SLASH, T_PERCENT})) {
            Token* tok = previous();
            Expr* right = unary();
            BinaryExpr::BinaryOp op;
            switch (tok->token_type) {
                case T_STAR:    op = BinaryExpr::MUL; break;
                case T_SLASH:   op = BinaryExpr::DIV; break;
                case T_PERCENT:   op = BinaryExpr::MODULO; break;
            }
            expr = new BinaryExpr(tok, expr, op, right);
        }
        return expr;
    }

    Expr* unary() {
        while (match({T_BANG, T_MINUS, T_AMP})) {
            Token* tok = previous();
            Expr* right = unary();
            UnaryExpr::UnaryOp op;
            switch (tok->token_type) {
                case T_BANG:    op = UnaryExpr::LOGIC_NOT; break;
                case T_MINUS:   op = UnaryExpr::MINUS; break;
                case T_AMP:
                    op = UnaryExpr::ADDRESS_OF;
                    error(tok, "address-of op unsupported");
                    break;
            }
            return new UnaryExpr(tok, op, right);
        }
        return call();
    }

    Expr* call() {
        Expr* expr = nullptr;
        if (match(T_IDENT)) {
            Token* tok = previous();
            if (match(T_LPAREN)) {
                expr = finish_call(tok);
            } else if (match(T_LBRACK)) {
                Expr* data_index = expression();
                eat(T_RBRACK, "Expected ']' after array index");
                if (match(T_DOT)) {
                    Token* prop_tok = eat(T_IDENT, "Expected property name after '.'");
                    Expr* arr_index = nullptr;
                    if (match(T_LBRACK)) {
                        arr_index = expression();
                        eat(T_RBRACK, "Expected ']' after array index");
                    }
                    expr = new CdbExpr(tok,
                        tok->text, data_index, prop_tok->text, arr_index);
                } else {
                    expr = new ArrayExpr(tok, tok->text, data_index);
                }
            } else {
                expr = new VariableExpr(tok, tok->text);
            }
        } else {
            expr = primary();
        }

        return expr;
    }

    Expr* finish_call(Token* callee) {
        std::vector<Expr*> args;
        
        if (!check(T_RPAREN)) do {
            args.push_back(expression());
        } while (match(T_COMMA));

        eat(T_RPAREN, "Expected ')' after argument list");

        return new CallExpr(callee, callee->text, args);
    }

    Expr* primary() {
        if (match(T_NUMBER))
            return new IntLiteralExpr(previous(), std::stoi(previous()->text, nullptr, 0));
        if (match(T_STRING))
            return new StrLiteralExpr(previous(), previous()->text);
        if (match(T_F_QUOTE)) {
            std::vector<FStringExpr::Fragment> frags;
            while (!is_at_end() && !check(T_F_QUOTE)) {
                if (match(T_STRING)) frags.push_back({previous()->text, nullptr});
                else if (match(T_LBRACE)) {
                    frags.push_back({"", expression()});
                    eat(T_RBRACE, "Expected '}' after f-string expression");
                }
                else error(peek(), "Unexpected token in f-string");
            }
            eat(T_F_QUOTE, "Expected ending quote after f-string");
            return new FStringExpr(previous(), frags);
        }
        if (match(T_LPAREN)) {
            Expr* expr = expression();
            eat(T_RPAREN, "Expected ')' after expression");
            return expr;
        }
        error(peek(), "Invalid expression");
        return nullptr;
    }

    // utility
    bool is_at_end() {
        return peek()->token_type == T_EOF;
    }

    bool check(TokType t) {
        return peek()->token_type == t;
    }

    Token* eat(TokType t, std::string error_msg) {
        if (check(t)) {
            return advance();
        }
        else error(peek(), error_msg);
        return nullptr;
    }

    bool match(TokType t) {
        if (check(t)) {
            advance();
            return true;
        }
        return false; 
    }

    bool match(std::initializer_list<TokType> t_list) {
        for (const TokType &t : t_list) {
            if (check(t)) {
                advance();
                return true;
            }
        }
        return false; 
    }

    Token* peek() {
        return &tokens.at(pos);
    }

    Token* advance() {
        if (!is_at_end()) pos++;
        return previous();
    }

    Token* previous() {
        return &tokens.at(pos - 1);
    }

    void synchronize() {
        advance();

        while (!is_at_end()) {
            if (previous()->token_type == T_SEMICOLON) return;
            advance();
        }
    }

    void error(Token* t, std::string error_msg) {
        had_error = true;
        std::cout << "parse error " << t->file << ": line " << t->line << " " << t->text << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};