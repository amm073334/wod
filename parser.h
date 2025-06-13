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
        Token* type = previous();
        Token* name = eat(T_IDENT, "Expected function name");
        eat(T_LPAREN, "Expected '(' after function name");
        std::vector<FunctionStmt::ParamDecl> params;
        size_t n_int_args = 0;
        size_t n_str_args = 0;
        if (!check(T_RPAREN)) do {
            if (n_int_args > 5 || n_str_args > 5)
                error(peek(), "Too many parameters (max: 5)");
            if (match({T_INT, T_STR})) {
                params.push_back({previous(), eat(T_IDENT, "Expected parameter name")});
                previous()->token_type == T_INT ? n_int_args++ : n_str_args++;
            }
        } while (match(T_COMMA));

        eat(T_RPAREN, "Expected ')' after function parameters");
        eat(T_LBRACE, "Expected '{' before function body");
        return new FunctionStmt(type, name, params, block());
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
        if (match(T_INT)) return var_stmt();
        if (match(T_LBRACE)) return new BlockStmt(block());
        if (match(T_IDENT)) return assign_stmt();

        return expr_stmt();
    }

    Stmt* var_stmt() {
        std::vector<Token*> qualifiers = {previous()};
        Token* name = eat(T_IDENT, "Expected variable name");

        Expr* initializer = nullptr;
        if (match(T_EQUAL)) initializer = expression();

        eat(T_SEMICOLON, "Expected ';' after variable declaration.");
        return new VarStmt(qualifiers, name, initializer);
    }

    Stmt* if_stmt() {
        Token* keyword = previous();
        eat(T_LPAREN, "Expected '(' after 'if'");
        Expr* condition = expression();
        eat(T_RPAREN, "Expected ')' after if condition");
        Stmt* then_branch = statement();
        Stmt* else_branch = nullptr;
        if (match(T_ELSE)) else_branch = statement();
        return new IfStmt(keyword, condition, then_branch, else_branch);
    }

    Stmt* loop_stmt() {
        Token* keyword = previous();
        Expr* count = nullptr;
        if (match(T_LPAREN)) {
            count = expression();
            eat(T_RPAREN, "Expected ')' after loop count");
        }
        Stmt* body = statement();
        return new LoopStmt(keyword, count, body);
    }

    Stmt* return_stmt() {
        Token* keyword = previous();
        Expr* expr = nullptr;
        if (!check(T_SEMICOLON)) expr = expression();
        eat(T_SEMICOLON, "Expected ';' after return statement");
        return new ReturnStmt(keyword, expr);
    }

    Stmt* assign_stmt() {
        Token* name = previous();
        eat(T_EQUAL, "Expected assignment statement containing '='");
        Expr* expr = expression();
        eat(T_SEMICOLON, "Expected ';' after assignment statement");
        return new AssignStmt(name, expr);
    }

    Stmt* expr_stmt() {
        Expr* expr = expression();
        eat(T_SEMICOLON, "Expected ';' after expression");
        return new ExprStmt(expr);
    }

    // expressions
    Expr* expression() {
        return or();
    }

    #define WOD_PARSE_LEFT_BINOP(NEXT_PREC, ...) do {   \
        Expr* expr = NEXT_PREC();                       \
        while (match(__VA_ARGS__)) {                    \
            Token* op = previous();                     \
            Expr* right = NEXT_PREC();                  \
            expr = new BinaryExpr(expr, op, right);     \
        }                                               \
        return expr;                                    \
    } while (0)

    Expr* or() {
        WOD_PARSE_LEFT_BINOP(and, T_PIPE_PIPE);
    }

    Expr* and() {
        WOD_PARSE_LEFT_BINOP(bit_or, T_AMP_AMP);
    }

    Expr* bit_or() {
        WOD_PARSE_LEFT_BINOP(bit_and, T_PIPE);
    }

    Expr* bit_and() {
        WOD_PARSE_LEFT_BINOP(comparison, T_AMP);
    }

    Expr* comparison() {
        WOD_PARSE_LEFT_BINOP(bit_shift, {T_GREATER, T_GREATER_EQUAL, T_LESS, T_LESS_EQUAL});
    }

    Expr* bit_shift() {
        WOD_PARSE_LEFT_BINOP(addsub, {T_LESS_LESS, T_GREATER_GREATER});
    }
    
    Expr* addsub() {
        WOD_PARSE_LEFT_BINOP(muldiv, {T_PLUS, T_MINUS});
    }

    Expr* muldiv() {
        WOD_PARSE_LEFT_BINOP(unary, {T_STAR, T_SLASH});
    }

    #undef WOD_PARSE_LEFT_BINOP

    Expr* unary() {
        while (match({T_BANG, T_MINUS})) {
            Token* op = previous();
            Expr* right = unary();
            return new UnaryExpr(op, right);
        }

        return call();
    }

    Expr* call() {
        Expr* expr = nullptr;
        if (match(T_IDENT)) {
            Token* name = previous();
            expr = new VariableExpr(name);
            if (match(T_LPAREN)) {
                expr = finish_call(name);
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

        return new CallExpr(callee, args);
    }

    Expr* primary() {
        if (match(T_NUMBER)) return new IntLiteralExpr(std::stoi(previous()->text));
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