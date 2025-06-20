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
            if (match(T_CONST)) return var_stmt();
            if (match(T_CDB)) return cdb_stmt();
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
                case T_INT: field_type = WodType{WodType::TYPE_INT}; break;
                case T_STR: field_type = WodType{WodType::TYPE_STR}; break;
            }
            Token* ntok = eat(T_IDENT, "Expected field name");
            
            Expr* initializer = nullptr;
            if (match(T_EQUAL)) {
                initializer = expression();
                error(ntok, "CDB field initializers are not yet supported");
            }
            fields.push_back(
                new VarStmt{{ntok->line, ntok->col, ntok->file},
                false, field_type, ntok->text, initializer});
            eat(T_SEMICOLON, "Expected ';' after cdb field");
        }
        eat(T_RBRACE, "Expected '}' at end of cdb declaration");

        return new CdbStmt({tok->line, tok->col, tok->file}, tok->text, fields);
    }

    Stmt* function_decl() {
        bool is_inline = false;
        if (previous()->token_type == T_INLINE) {
            is_inline = true;
            advance();
        }

        WodType type;
        switch (previous()->token_type) {
            case T_VOID: type = WodType{WodType::TYPE_VOID}; break;
            case T_INT:  type = WodType{WodType::TYPE_INT}; break;
            case T_STR:  type = WodType{WodType::TYPE_STR}; break;
            default: error(previous(), "Invalid function return type"); break;
        }
        Token* tok = eat(T_IDENT, "Expected function name");
        eat(T_LPAREN, "Expected '(' after function name");
        std::vector<VarStmt*> params;
        size_t n_int_args = 0;
        size_t n_str_args = 0;
        if (!check(T_RPAREN)) do {
            if (n_int_args > 5 || n_str_args > 5)
                error(peek(), "Too many parameters (max: 5)");
            if (match({T_INT, T_STR})) {
                WodType param_type;
                switch(previous()->token_type) {
                    case T_INT: param_type = WodType{WodType::TYPE_INT}; break;
                    case T_STR: param_type = WodType{WodType::TYPE_STR}; break;
                }
                Token* ntok = eat(T_IDENT, "Expected parameter name");
                params.push_back(new VarStmt{{ntok->line, ntok->col, ntok->file}, false, param_type, ntok->text, nullptr});
                if (param_type == WodType{WodType::TYPE_INT}) n_int_args++;
                else n_str_args++;
            } else {
                error(peek(), "Invalid parameter type");
            }
        } while (match(T_COMMA));

        eat(T_RPAREN, "Expected ')' after function parameters");
        eat(T_LBRACE, "Expected '{' before function body");
        return new FunctionStmt({tok->line, tok->col}, type, tok->text, params, block(), is_inline);
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
        if (match(T_CONTINUE)) return new ContinueStmt({previous()->line, previous()->col, previous()->file});
        if (match(T_BREAK)) return new BreakStmt({previous()->line, previous()->col, previous()->file});
        if (match({T_INT, T_STR, T_CONST})) return var_stmt();
        if (match(T_CMD)) return cmd_stmt();
        if (match(T_LBRACE)) return new BlockStmt({previous()->line, previous()->col, previous()->file}, block());

        return expr_stmt();
    }

    Stmt* while_stmt() {
        Token* tok = previous();
        Position pos = {tok->line, tok->col, tok->file};
        eat(T_LPAREN, "Expected '(' after 'while'");
        Expr* condition = expression();
        eat(T_RPAREN, "Expected ')' after while condition");
        Stmt* body = statement();
        Stmt* if_wrapper = new IfStmt(pos, condition, body, new BreakStmt(pos));
        return new LoopStmt(pos, nullptr, if_wrapper);
    }

    Stmt* for_stmt() {
        Token* tok = previous();
        Position pos = {tok->line, tok->col, tok->file};
        eat(T_LPAREN, "Expected '(' after 'for'");

        Stmt* initializer;
        if (match(T_SEMICOLON)) initializer = nullptr;
        else if (match({T_INT, T_STR})) initializer = var_stmt();
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
        
        if (increment) body = new BlockStmt(pos, {body, new ExprStmt(pos, increment)});            
        if (condition) body = new IfStmt(pos, condition, body, new BreakStmt(pos));
        body = new LoopStmt(pos, nullptr, body);
        
        if (initializer) body = new BlockStmt(pos, {initializer, body});

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
        
        return new CmdStmt({tok->line, tok->col, tok->file}, cmd_id, int_fields, str_fields);
    }

    Stmt* var_stmt() {
        bool is_const = false;
        if (previous()->token_type == T_CONST) {
            is_const = true;
            advance();
        }
        WodType type;
        switch (previous()->token_type) {
            case T_INT: type = WodType{WodType::TYPE_INT}; break;
            case T_STR: type = WodType{WodType::TYPE_STR}; break;
        }
        Token* tok = eat(T_IDENT, "Expected variable name");

        Expr* initializer = nullptr;
        if (match(T_EQUAL)) initializer = expression();

        eat(T_SEMICOLON, "Expected ';' after variable declaration");
        return new VarStmt({tok->line, tok->col, tok->file}, is_const, type, tok->text, initializer);
    }

    Stmt* if_stmt() {
        Token* tok = previous();
        eat(T_LPAREN, "Expected '(' after 'if'");
        Expr* condition = expression();
        eat(T_RPAREN, "Expected ')' after if condition");
        Stmt* then_branch = statement();
        Stmt* else_branch = nullptr;
        if (match(T_ELSE)) else_branch = statement();
        return new IfStmt({tok->line, tok->col, tok->file}, condition, then_branch, else_branch);
    }

    Stmt* loop_stmt() {
        Token* tok = previous();
        Expr* count = nullptr;
        if (match(T_LPAREN)) {
            count = expression();
            eat(T_RPAREN, "Expected ')' after loop count");
        }
        Stmt* body = statement();
        return new LoopStmt({tok->line, tok->col, tok->file}, count, body);
    }

    Stmt* return_stmt() {
        Token* tok = previous();
        Expr* expr = nullptr;
        if (!check(T_SEMICOLON)) expr = expression();
        eat(T_SEMICOLON, "Expected ';' after return statement");
        return new ReturnStmt({tok->line, tok->col, tok->file}, expr);
    }

    Stmt* expr_stmt() {
        Token* tok = peek();
        Expr* expr = expression();
        eat(T_SEMICOLON, "Expected ';' after expression");
        return new ExprStmt({tok->line, tok->col, tok->file}, expr);
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
            return new AssignExpr({tok->line, tok->col, tok->file}, expr, op, value);
        }
        return expr;
    }

    Expr* or() {
        Expr* expr = and();
        while (match(T_PIPE_PIPE)) {
            Token* tok = previous();
            Expr* right = and();
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, BinaryExpr::LOGIC_OR, right);
        }
        return expr;
    }

    Expr* and() {
        Expr* expr = bit_or();
        while (match(T_AMP_AMP)) {
            Token* tok = previous();
            Expr* right = bit_or();
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, BinaryExpr::LOGIC_AND, right);
        }
        return expr;
    }

    Expr* bit_or() {
        Expr* expr = bit_xor();
        while (match(T_PIPE)) {
            Token* tok = previous();
            Expr* right = bit_xor();
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, BinaryExpr::BIT_OR, right);
        }
        return expr;
    }

    Expr* bit_xor() {
        Expr* expr = bit_and();
        while (match(T_CARET)) {
            Token* tok = previous();
            Expr* right = bit_and();
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, BinaryExpr::BIT_XOR, right);
        }
        return expr;
    }

    Expr* bit_and() {
        Expr* expr = equality();
        while (match(T_AMP)) {
            Token* tok = previous();
            Expr* right = equality();
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, BinaryExpr::BIT_AND, right);
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
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, op, right);
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
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, op, right);
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
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, op, right);
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
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, op, right);
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
            expr = new BinaryExpr({tok->line, tok->col, tok->file}, expr, op, right);
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
                case T_AMP:     op = UnaryExpr::ADDRESS_OF; break;
            }
            return new UnaryExpr({tok->line, tok->col, tok->file}, op, right);
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
                eat(T_RBRACK, "Expected ']' after cdb data index");
                eat(T_DOT, "Expected '.' after ']'");
                Token* prop_tok = eat(T_IDENT, "Expected property name after '.'");
                expr = new CdbExpr({tok->line, tok->col, tok->file},
                    tok->text, data_index, prop_tok->text);
            } else {
                expr = new VariableExpr({tok->line, tok->col, tok->file}, tok->text);
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

        return new CallExpr({callee->line, callee->col, callee->file}, callee->text, args);
    }

    Expr* primary() {
        if (match(T_NUMBER))
            return new IntLiteralExpr({previous()->line, previous()->col, previous()->file}, std::stoi(previous()->text, nullptr, 0));
        if (match(T_STRING))
            return new StrLiteralExpr({previous()->line, previous()->col, previous()->file}, previous()->text);
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
            return new FStringExpr({previous()->line, previous()->col, previous()->file}, frags);
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
        std::cout << "parse error " << t->file << ": line " << t->line << " " << t->text << " " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};