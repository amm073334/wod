#pragma once

enum TokType {
    // one character
    T_LBRACE, T_RBRACE,
    T_LPAREN, T_RPAREN,
    T_LBRACK, T_RBRACK,
    T_COMMA, T_DOT,
    T_CARET,
    T_SEMICOLON,

    // one or two characters
    T_PLUS, T_PLUS_EQUAL,
    T_MINUS, T_MINUS_EQUAL,
    T_SLASH, T_SLASH_EQUAL,
    T_STAR, T_STAR_EQUAL,
    T_PERCENT, T_PERCENT_EQUAL,
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
    T_WHILE, T_FOR,
    T_CMD,
    T_CDB, T_TYPEDEF,

    // literals
    T_IDENT, T_NUMBER, T_STRING,

    // eof
    T_EOF
};

struct Token {
    TokType token_type;
    std::string text;
    size_t line;
    size_t col;
    std::string file;

    std::string to_string() {
        return "(" + std::to_string(token_type) + " " + text + ")";
    }
};