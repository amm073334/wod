#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer.h"

typedef struct {
    const char *name;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {.name = "import",   .type = TOK_IMPORT},
    {.name = "as",       .type = TOK_AS},
    {.name = "void",     .type = TOK_VOID},
    {.name = "int",      .type = TOK_INT},
    {.name = "str",      .type = TOK_STR},
    {.name = "bool",     .type = TOK_BOOL},
    {.name = "const",    .type = TOK_CONST},
    {.name = "inline",   .type = TOK_INLINE},
    {.name = "if",       .type = TOK_IF},
    {.name = "else",     .type = TOK_ELSE},
    {.name = "loop",     .type = TOK_LOOP},
    {.name = "return",   .type = TOK_RETURN},
    {.name = "continue", .type = TOK_CONTINUE},
    {.name = "break",    .type = TOK_BREAK},
    {.name = "while",    .type = TOK_WHILE},
    {.name = "for",      .type = TOK_FOR},
    {.name = "in",       .type = TOK_IN},
    {.name = "true",     .type = TOK_TRUE},
    {.name = "false",    .type = TOK_FALSE},
    {.name = "cmd",      .type = TOK_CMD},
    {.name = "cdb",      .type = TOK_CDB},
    {.name = "cevtype",  .type = TOK_CEVTYPE},
    {.name = "apply",    .type = TOK_APPLY},
};

void lexer_init(Lexer *lexer, Source source) {
    lexer->source = source;
    lexer->start = source.text.data;
    lexer->current = source.text.data;
    lexer->line = 1;
    lexer->col = 1;
    lexer->interpolation_depth = 0;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool is_dec_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_bin_digit(char c) {
    return c >= '0' && c <= '1';
}

static bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

static bool is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer *lexer) {
    lexer->current++;
    lexer->col++;
    return lexer->current[-1];
}

static bool match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    advance(lexer);
    return true;
}

static char peek(Lexer *lexer) {
    return *lexer->current;
}

static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static Token make_token(Lexer *lexer, TokenType type) {
    size_t len = lexer->current - lexer->start;
    return (Token){
        .type = type,
        .text.data = lexer->start,
        .text.len = len,
        .loc = {
            .source = lexer->source,
            .line = lexer->line,
            .column = lexer->col - len
        },
    };
}

static Token error_token(Lexer *lexer, const char *message) {
    return (Token){
        .type = TOK_ERROR,
        .text.data = message,
        .text.len = strlen(message),
        .loc = {
            .source = lexer->source,
            .line = lexer->line,
            .column = lexer->col - 1,
        },
    };
}

static Token string(Lexer *lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        if (c == '"') {
            // Move past end quote.
            advance(lexer);

            return make_token(lexer, TOK_STRING);
        }

        if (c == '\r' || c == '\n')
            return error_token(lexer, "Unterminated string.");
    
        if (c == '$' && peek_next(lexer) == '{') {
            lexer->interpolation_depth++;

            // Make the token before advancing past the
            // dollar sign so that it's not included in the token.
            Token token = make_token(lexer, TOK_INTERPOLATION);
            advance(lexer);

            // Skip the bracket.
            advance(lexer);

            return token;
        }

        advance(lexer);
    }

    return error_token(lexer, "Unterminated string.");
}

static TokenType identifier_type(Lexer *lexer) {
    // TODO: This can be made more efficient with string views and tries,
    //       but they're harder to deal with, so stick with a dumb linear
    //       search for now.
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        size_t len = strlen(keywords[i].name);
        if (lexer->current - lexer->start == (int)len &&
            memcmp(lexer->start, keywords[i].name, len) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENTIFIER;
}

static Token identifier(Lexer *lexer) {
    while (is_alpha(peek(lexer)) || is_dec_digit(peek(lexer)))
        advance(lexer);
    
    return make_token(lexer, identifier_type(lexer));
}

static Token decimal(Lexer *lexer) {
    while (is_dec_digit(peek(lexer))) advance(lexer);
    return make_token(lexer, TOK_NUMBER);    
}

static Token binary(Lexer *lexer) {
    while (is_bin_digit(peek(lexer))) advance(lexer);
    return make_token(lexer, TOK_NUMBER);    
}

static Token hexadecimal(Lexer *lexer) {
    while (is_hex_digit(peek(lexer))) advance(lexer);
    return make_token(lexer, TOK_NUMBER);    
}

static void skip_whitespace(Lexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                advance(lexer);
                lexer->line++;
                lexer->col = 1;
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    while (peek(lexer) != '\n' && !is_at_end(lexer))
                        advance(lexer);
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token scan_token(Lexer *lexer) {
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (is_at_end(lexer))
        return make_token(lexer, TOK_EOF);

    char c = advance(lexer);
    if (is_alpha(c)) return identifier(lexer);
    if (is_dec_digit(c)) {
        if (c == '0') {
            if (match(lexer, 'x'))
                return hexadecimal(lexer);
            else if (match(lexer, 'b'))
                return binary(lexer);

            if (is_dec_digit(peek(lexer)))
                return error_token(lexer, "Decimal numbers cannot start with '0'.");
        }
        return decimal(lexer);
    }

    switch (c) {
        case '(': return make_token(lexer, TOK_LEFT_PAREN);
        case ')': return make_token(lexer, TOK_RIGHT_PAREN);
        case '{': return make_token(lexer, TOK_LEFT_BRACE);
        case '}':
            if (lexer->interpolation_depth > 0) {
                lexer->interpolation_depth--;
                return string(lexer);
            }
            return make_token(lexer, TOK_RIGHT_BRACE);
        case '[': return make_token(lexer, TOK_LEFT_BRACK);
        case ']': return make_token(lexer, TOK_RIGHT_BRACK);
        case ',': return make_token(lexer, TOK_COMMA);
        case '.':
            return make_token(lexer,
                match(lexer, '.') ? TOK_DOT_DOT : TOK_DOT);
        case '^': return make_token(lexer, TOK_CARET);
        case ';': return make_token(lexer, TOK_SEMICOLON);
        case '+':
            return make_token(lexer,
                match(lexer, '=') ? TOK_PLUS_EQUAL : TOK_PLUS);
        case '-':
            return make_token(lexer,
                match(lexer, '=') ? TOK_MINUS_EQUAL : TOK_MINUS);
        case '/':
            return make_token(lexer,
                match(lexer, '=') ? TOK_SLASH_EQUAL : TOK_SLASH);
        case '*':
            return make_token(lexer,
                match(lexer, '=') ? TOK_STAR_EQUAL : TOK_STAR);
        case '%':
            return make_token(lexer,
                match(lexer, '=') ? TOK_PERCENT_EQUAL : TOK_PERCENT);
        case '!':
            return make_token(lexer,
                match(lexer, '=') ? TOK_BANG_EQUAL : TOK_BANG);
        case '=':
            if (match(lexer, '='))
                return make_token(lexer, TOK_EQUAL_EQUAL);
            else return make_token(lexer, TOK_EQUAL);
        case '<':
            if (match(lexer, '='))
                return make_token(lexer, TOK_LESS_EQUAL);
            else if (match(lexer, '<'))
                return make_token(lexer, TOK_LESS_LESS);
            else
                return make_token(lexer, TOK_LESS);
        case '>':
            if (match(lexer, '='))
                return make_token(lexer, TOK_GREATER_EQUAL);
            else if (match(lexer, '>'))
                return make_token(lexer, TOK_GREATER_GREATER);
            else
                return make_token(lexer, TOK_GREATER);
        case '&':
            if (match(lexer, '='))
                return make_token(lexer, TOK_AMP_EQUAL);
            else if (match(lexer, '&'))
                return make_token(lexer, TOK_AMP_AMP);
            else
                return make_token(lexer, TOK_AMP);
        case '|':
            if (match(lexer, '='))
                return make_token(lexer, TOK_PIPE_EQUAL);
            else if (match(lexer, '|'))
                return make_token(lexer, TOK_PIPE_PIPE);
            else
                return make_token(lexer, TOK_PIPE);
        case '"':
            return string(lexer);
    }

    return error_token(lexer, "Unexpected character.");
}