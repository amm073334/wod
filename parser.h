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
            if (match({T_VOID, T_INT, T_STR})) return function_decl();
            error(peek(), "Invalid statement");
        } catch (std::runtime_error e) {
            synchronize();
            return nullptr;
        }
        return nullptr;
    }

    Stmt* function_decl() {
        WodType type;
        switch (previous()->token_type) {
            case T_VOID: type = TYPE_VOID; break;
            case T_INT:  type = TYPE_INT; break;
            case T_STR:  type = TYPE_STR; break;
        }
        Token* tok = eat(T_IDENT, "Expected function name");
        eat(T_LPAREN, "Expected '(' after function name");
        std::vector<FunctionStmt::ParamDecl> params;
        size_t n_int_args = 0;
        size_t n_str_args = 0;
        if (!check(T_RPAREN)) do {
            if (n_int_args > 5 || n_str_args > 5)
                error(peek(), "Too many parameters (max: 5)");
            if (match({T_INT, T_STR})) {
                WodType arg_type;
                switch(previous()->token_type) {
                    case T_INT: arg_type = TYPE_INT; break;
                    case T_STR: arg_type = TYPE_STR; break;
                }
                params.push_back({arg_type, eat(T_IDENT, "Expected parameter name")->text});
                previous()->token_type == T_INT ? n_int_args++ : n_str_args++;
            }
        } while (match(T_COMMA));

        eat(T_RPAREN, "Expected ')' after function parameters");
        eat(T_LBRACE, "Expected '{' before function body");
        return new FunctionStmt({tok->line, tok->col}, type, tok->text, params, block());
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
        if (match(T_RETURN)) return return_stmt();
        if (match({T_INT, T_STR})) return var_stmt();
        if (match(T_LBRACE)) return new BlockStmt({previous()->line, previous()->col}, block());

        return expr_stmt();
    }

    Stmt* var_stmt() {
        WodType type;
        switch (previous()->token_type) {
            case T_INT: type = TYPE_INT; break;
            case T_STR: type = TYPE_STR; break;
        }
        Token* tok = eat(T_IDENT, "Expected variable name");

        Expr* initializer = nullptr;
        if (match(T_EQUAL)) initializer = expression();

        eat(T_SEMICOLON, "Expected ';' after variable declaration.");
        return new VarStmt({tok->line, tok->col}, type, tok->text, initializer);
    }

    Stmt* if_stmt() {
        Token* tok = previous();
        eat(T_LPAREN, "Expected '(' after 'if'");
        Expr* condition = expression();
        eat(T_RPAREN, "Expected ')' after if condition");
        Stmt* then_branch = statement();
        Stmt* else_branch = nullptr;
        if (match(T_ELSE)) else_branch = statement();
        return new IfStmt({tok->line, tok->col}, condition, then_branch, else_branch);
    }

    Stmt* loop_stmt() {
        Token* tok = previous();
        Expr* count = nullptr;
        if (match(T_LPAREN)) {
            count = expression();
            eat(T_RPAREN, "Expected ')' after loop count");
        }
        Stmt* body = statement();
        return new LoopStmt({tok->line, tok->col}, count, body);
    }

    Stmt* return_stmt() {
        Token* tok = previous();
        Expr* expr = nullptr;
        if (!check(T_SEMICOLON)) expr = expression();
        eat(T_SEMICOLON, "Expected ';' after return statement");
        return new ReturnStmt({tok->line, tok->col}, expr);
    }

    Stmt* expr_stmt() {
        Token* tok = peek();
        Expr* expr = expression();
        eat(T_SEMICOLON, "Expected ';' after expression");
        return new ExprStmt({tok->line, tok->col}, expr);
    }

    // expressions
    Expr* expression() {
        return assignment();
    }

    Expr* assignment() {
        Expr* expr = or();
        if (match(T_EQUAL)) {
            Token* tok = previous();
            Expr* value = or();
            return new AssignExpr({tok->line, tok->col}, expr, value);
        }
        return expr;
    }

    Expr* or() {
        Expr* expr = and();
        while (match(T_PIPE_PIPE)) {
            Token* tok = previous();
            Expr* right = and();
            expr = new BinaryExpr({tok->line, tok->col}, expr, BinaryExpr::LOGIC_OR, right);
        }
        return expr;
    }

    Expr* and() {
        Expr* expr = bit_or();
        while (match(T_AMP_AMP)) {
            Token* tok = previous();
            Expr* right = bit_or();
            expr = new BinaryExpr({tok->line, tok->col}, expr, BinaryExpr::LOGIC_AND, right);
        }
        return expr;
    }

    Expr* bit_or() {
        Expr* expr = bit_and();
        while (match(T_PIPE)) {
            Token* tok = previous();
            Expr* right = bit_and();
            expr = new BinaryExpr({tok->line, tok->col}, expr, BinaryExpr::BIT_OR, right);
        }
        return expr;
    }

    Expr* bit_and() {
        Expr* expr = comparison();
        while (match(T_AMP)) {
            Token* tok = previous();
            Expr* right = comparison();
            expr = new BinaryExpr({tok->line, tok->col}, expr, BinaryExpr::BIT_AND, right);
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
            expr = new BinaryExpr({tok->line, tok->col}, expr, op, right);
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
            expr = new BinaryExpr({tok->line, tok->col}, expr, op, right);
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
            expr = new BinaryExpr({tok->line, tok->col}, expr, op, right);
        }
        return expr;
    }

    Expr* muldiv() {
        Expr* expr = unary();
        while (match({T_STAR, T_SLASH})) {
            Token* tok = previous();
            Expr* right = unary();
            BinaryExpr::BinaryOp op;
            switch (tok->token_type) {
                case T_STAR:    op = BinaryExpr::MUL; break;
                case T_SLASH:   op = BinaryExpr::DIV; break;
            }
            expr = new BinaryExpr({tok->line, tok->col}, expr, op, right);
        }
        return expr;
    }

    Expr* unary() {
        while (match({T_BANG, T_MINUS})) {
            Token* tok = previous();
            Expr* right = unary();
            UnaryExpr::UnaryOp op;
            switch (tok->token_type) {
                case T_BANG:    op = UnaryExpr::LOGIC_NOT; break;
                case T_MINUS:   op = UnaryExpr::MINUS; break;
            }
            return new UnaryExpr({tok->line, tok->col}, op, right);
        }
        return call();
    }

    Expr* call() {
        Expr* expr = nullptr;
        if (match(T_IDENT)) {
            Token* tok = previous();
            if (match(T_LPAREN)) {
                expr = finish_call(tok);
            } else {
                expr = new VariableExpr({tok->line, tok->col}, tok->text);
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

        return new CallExpr({callee->line, callee->col}, callee->text, args);
    }

    Expr* primary() {
        if (match(T_NUMBER))
            return new IntLiteralExpr({previous()->line, previous()->col}, std::stoi(previous()->text));
        if (match(T_STRING))
            return new StrLiteralExpr({previous()->line, previous()->col}, previous()->text);
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

    bool check(TokenType t) {
        return peek()->token_type == t;
    }

    Token* eat(TokenType t, std::string error_msg) {
        if (check(t)) {
            return advance();
        }
        else error(peek(), error_msg);
        return nullptr;
    }

    bool match(TokenType t) {
        if (check(t)) {
            advance();
            return true;
        }
        return false; 
    }

    bool match(std::initializer_list<TokenType> t_list) {
        for (const TokenType &t : t_list) {
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
        std::cout << "parse error: line " << t->line << " " << t->text << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};