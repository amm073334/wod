#ifndef WOD_LEXER_H_
#define WOD_LEXER_H_

#include "common.h"
#include "source.h"
#include "location.h"

typedef enum {
    // One character.
    TOK_LEFT_PAREN, TOK_RIGHT_PAREN,
    TOK_LEFT_BRACE, TOK_RIGHT_BRACE,
    TOK_LEFT_BRACK, TOK_RIGHT_BRACK,
    TOK_COMMA,
    TOK_CARET,
    TOK_SEMICOLON,

    // One or two characters.
    TOK_PLUS, TOK_PLUS_EQUAL, TOK_PLUS_PLUS,
    TOK_MINUS, TOK_MINUS_EQUAL, TOK_MINUS_MINUS,
    TOK_SLASH, TOK_SLASH_EQUAL,
    TOK_STAR, TOK_STAR_EQUAL,
    TOK_PERCENT, TOK_PERCENT_EQUAL,
    TOK_BANG, TOK_BANG_EQUAL,
    TOK_EQUAL, TOK_EQUAL_EQUAL,
    TOK_LESS, TOK_LESS_EQUAL, TOK_LESS_LESS,
    TOK_GREATER, TOK_GREATER_EQUAL, TOK_GREATER_GREATER,
    TOK_AMP, TOK_AMP_EQUAL, TOK_AMP_AMP,
    TOK_PIPE, TOK_PIPE_EQUAL, TOK_PIPE_PIPE,
    TOK_DOT, TOK_DOT_DOT,
    
    TOK_DOLLAR_BRACE,

    // Keywords.
    TOK_IMPORT, TOK_AS,
    TOK_VOID, TOK_INT, TOK_STR, TOK_BOOL, TOK_CONST, TOK_INLINE,
    TOK_IF, TOK_ELSE, TOK_LOOP, TOK_RETURN, TOK_CONTINUE, TOK_BREAK,
    TOK_WHILE, TOK_FOR,
    TOK_TRUE, TOK_FALSE,
    TOK_CMD,
    TOK_UDB, TOK_CDB, TOK_CEVTYPE,
    TOK_APPLY, TOK_EXADDR,

    // Literals.
    TOK_IDENTIFIER, TOK_NUMBER, TOK_STRING,

    // Used for string interpolation.
    // Idea borrowed from https://github.com/wren-lang/wren/blob/main/src/vm/wren_compiler.c
    // (and described in https://github.com/munificent/craftinginterpreters/blob/master/note/answers/chapter16_scanning.md).
    TOK_INTERPOLATION,

    TOK_ERROR, TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    StringView text;
    Location loc;
} Token;

typedef struct {
    Source source;
    
    size_t interpolation_depth;

    const char *start;
    const char *current;
    size_t line;
    size_t col;
} Lexer;

void lexer_init(Lexer *lexer, Source source);
Token scan_token(Lexer *lexer);

#endif // WOD_LEXER_H_