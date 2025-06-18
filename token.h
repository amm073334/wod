#pragma once

enum TokenType {
    // one character
    T_LBRACE, T_RBRACE,
    T_LPAREN, T_RPAREN,
    T_LBRACK, T_RBRACK,
    T_COMMA, T_DOT,
    T_PLUS, T_MINUS,
    T_SLASH, T_STAR, T_PERCENT,
    T_CARET,
    T_SEMICOLON,

    // one or two characters
    T_BANG, T_BANG_EQUAL,
    T_EQUAL, T_EQUAL_EQUAL,
    T_LESS, T_LESS_EQUAL, T_LESS_LESS,
    T_GREATER, T_GREATER_EQUAL, T_GREATER_GREATER,
    T_AMP, T_AMP_AMP,
    T_PIPE, T_PIPE_PIPE,

    T_F_QUOTE,

    // keywords
    T_IMPORT,
    T_VOID, T_INT, T_STR, T_CONST, T_INLINE,
    T_IF, T_ELSE, T_LOOP, T_RETURN, T_CONTINUE, T_BREAK,
    T_CMD,

    // literals
    T_IDENT, T_NUMBER, T_STRING,

    // eof
    T_EOF
};

struct Token {
    TokenType token_type;
    std::string text;
    size_t line;
    size_t col;

    std::string to_string() {
        return "(" + std::to_string(token_type) + " " + text + ")";
    }
};