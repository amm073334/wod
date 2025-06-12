#pragma once

#include "types.h"

enum TokenType {
    // one character
    T_LBRACE, T_RBRACE,
    T_LPAREN, T_RPAREN,
    T_COMMA, T_DOT,
    T_PLUS, T_MINUS,
    T_SLASH, T_STAR, T_PERCENT,
    T_SEMICOLON,

    // one or two characters
    T_BANG, T_BANG_EQUAL,
    T_EQUAL, T_EQUAL_EQUAL,
    T_LESS, T_LESS_EQUAL, T_LESS_LESS,
    T_GREATER, T_GREATER_EQUAL, T_GREATER_GREATER,
    T_AMP, T_AMP_AMP,
    T_PIPE, T_PIPE_PIPE,

    // keywords
    T_VOID, T_INT, T_STR, T_RETURN,
    T_IF, T_ELSE, T_LOOP,

    // literals
    T_IDENT, T_NUMBER,

    // eof
    T_EOF
};

struct Token {
    Token(TokenType token_type, std::string text, int32_t n, size_t line) 
    : token_type(token_type), text(text), lit(n), line(line) {}
    Token(TokenType token_type, std::string text, std::string s, size_t line) 
    : token_type(token_type), text(text), lit(s), line(line) {}
    Token(TokenType token_type, std::string text, size_t line) 
    : token_type(token_type), text(text), line(line) {}

    const TokenType token_type;
    const std::string text;
    const LitVal lit;
    const size_t line;
    Type type;

    std::string to_string() {
        return "(" + std::to_string(token_type) + " " + text + ")";
    }
};