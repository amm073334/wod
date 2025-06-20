#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <fstream>
#include "token.h"

class Scanner {
public:
    std::vector<Token> scan_source(std::string src_file) {
        std::ifstream in(src_file, std::ios::binary | std::ios::ate);
        if (!in) {
            std::cout << "failed to open file " << src_file << std::endl;
            had_error = true;
            return {};
        }

        file = src_file;
        const size_t slash_pos = src_file.find_last_of("\\/");
        if (slash_pos != std::string::npos) base_dir = src_file.substr(0, slash_pos+1);

        // for some reason, doing a .reserve() on the member string and reading into that
        // causes 0 characters to be read, so using another string here is a bandaid
        size_t file_size = in.tellg();
        std::string src2(file_size, '\0');
        in.seekg(0);
        in.read(&src2[0], file_size);
        source = src2;

        while (!is_at_end()) {
            token_start = index;
            scan_token();
        }

        tokens.push_back({T_EOF, "", line, col, file});
        return tokens;
    }

    bool failed() { return had_error; }

private:
    std::string base_dir;
    std::vector<Token> tokens;
    std::string source;
    std::string file;
    size_t token_start = 0;
    size_t index = 0;
    size_t line = 1;
    size_t col = 1;
    bool had_error = false;
    bool finished_imports = false;
    bool scanned_import = false;
    size_t fstring_counter = 0;

    const std::unordered_map<std::string, TokenType> keywords = {
        {"import", T_IMPORT},
        {"void", T_VOID},
        {"int", T_INT},
        {"str", T_STR},
        {"const", T_CONST},
        {"inline", T_INLINE},
        {"if", T_IF},
        {"else", T_ELSE},
        {"loop", T_LOOP},
        {"while", T_WHILE},
        {"for", T_FOR},
        {"return", T_RETURN},
        {"continue", T_CONTINUE},
        {"break", T_BREAK},
        {"cmd", T_CMD},
        {"cdb", T_CDB},
    };

    void scan_token() {
        char c = advance();
        switch (c) {
            case '{': add_token(T_LBRACE); break;
            case '}':
                add_token(T_RBRACE);
                if (fstring_counter > 0) {
                    fstring_counter--;
                    fstring();
                }
                break;
            case '(': add_token(T_LPAREN); break;
            case ')': add_token(T_RPAREN); break;
            case '[': add_token(T_LBRACK); break;
            case ']': add_token(T_RBRACK); break;
            case ',': add_token(T_COMMA); break;
            case '.': add_token(T_DOT); break;
            case '+':
                if (match('=')) add_token(T_PLUS_EQUAL);
                else add_token(T_PLUS);
                break;
            case '-':
                if (match('=')) add_token(T_MINUS_EQUAL);
                else add_token(T_PLUS);
                break;
            case '*':
                if (match('=')) add_token(T_STAR_EQUAL);
                else add_token(T_STAR);
                break;
            case '^': add_token(T_CARET); break;
            case '%':
                if (match('=')) add_token(T_PERCENT_EQUAL);
                else add_token(T_PERCENT);
                break;
            case ';': add_token(T_SEMICOLON); break;
            case '!': add_token(match('=') ? T_BANG_EQUAL : T_BANG); break;
            case '=': add_token(match('=') ? T_EQUAL_EQUAL : T_EQUAL); break;
            case '<':
                if (match('=')) add_token(T_LESS_EQUAL);
                else if (match('<')) add_token(T_LESS_LESS);
                else add_token(T_LESS);
                break;
            case '>':
                if (match('=')) add_token(T_GREATER_EQUAL);
                else if (match('>')) add_token(T_GREATER_GREATER);
                else add_token(T_GREATER);
                break;
            case '&': add_token(match('&') ? T_AMP_AMP : T_AMP); break;
            case '|': add_token(match('|') ? T_PIPE_PIPE : T_PIPE); break;
            case '/':
                if (match('/')) while (peek() != '\n' && !is_at_end()) advance();
                else if (match('=')) add_token(T_SLASH_EQUAL);
                else add_token(T_SLASH);
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n': line++; col = 1; break;
            case '"': try_string(); break;
            case '0': try_hex(); break;
            case 'f':
                if (match('"')) {
                    add_token(T_F_QUOTE);
                    fstring();
                } else try_identifier();
                break;
            default:
                if (std::isdigit(c)) try_decimal();
                else if (is_alpha_under(c)) try_identifier();
                else error("Unexpected character");
                break;
        }
        if (!scanned_import) finished_imports = true;
        scanned_import = false;
    }

    void try_decimal() {
        while (std::isdigit(peek())) advance();
        add_token(T_NUMBER);
    }

    void try_hex() {
        if (!match('x')) {
            if (std::isdigit(peek())) {
                error("Numeric literals cannot start with 0");
                while (std::isdigit(peek())) advance();
            }
        } else {
            if (!std::isxdigit(peek())) {
                error("Invalid hex literal");
            }
            while (std::isxdigit(peek())) advance();
        }
        add_token(T_NUMBER);
    }

    void try_identifier() {
        while (is_alpha_under(peek()) || std::isdigit(peek())) advance();
        std::string text = source.substr(token_start, index - token_start);
        if (keywords.count(text)) {
            if (keywords.at(text) != T_IMPORT) {
                add_token(keywords.at(text));
                return;
            }

            if (finished_imports)
                error("Attempted to import after top of file");
            scanned_import = true;

            while (!is_at_end() && peek() == ' ') {
                advance();
            }

            if (is_at_end() || advance() != '"')
                error("Expected '\"' after import");
            size_t name_start = index;
            while (!is_at_end() && peek() != '"') {
                if (peek() == '\n') error("Unterminated string");
                advance();
            }
            if (is_at_end()) error("Unterminated string");
            std::string file_name = base_dir + source.substr(name_start, index - name_start) + ".wod";
            advance();
            if (advance() != ';') error("Expected ';' after import statement");
            
            Scanner s;
            std::vector<Token> imported = s.scan_source(file_name);
            if (s.failed()) had_error = true;
            tokens.insert(tokens.end(), std::make_move_iterator(imported.begin()), std::make_move_iterator(imported.end() - 1));
        } else {
            add_token(T_IDENT);
        }
    }

    void try_string() {
        while (peek() != '"' && peek() != '\n' && !is_at_end()) {
            advance();
        }
        if (is_at_end() || peek() == '\n') {
            error("Unterminated string");
        }
        tokens.push_back({T_STRING, source.substr(token_start + 1, index - token_start - 1), line, col, file});
        advance();
    }

    void fstring() {
        token_start = index;
        while (peek() != '"' && peek() != '\n' && peek() != '{' && !is_at_end()) {
            advance();
        }
        if (is_at_end() || peek() == '\n') {
            error("Unterminated string");
        }
        tokens.push_back({T_STRING, source.substr(token_start, index - token_start), line, col, file});
        
        token_start = index;
        if (advance() == '{') {
            add_token(T_LBRACE);
            fstring_counter++;
        } else if (fstring_counter == 0)
            add_token(T_F_QUOTE);
    }

    // utility
    bool is_alpha_under(char c) {
        return std::isalpha(c) || c == '_';
    }

    char advance() {
        col++;
        return source.at(index++);
    }

    bool match(char expected) {
        if (is_at_end()) return false;
        if (source.at(index) != expected) return false;
        advance();
        return true;
    }

    char peek() {
        if (is_at_end()) return '\0';
        return source.at(index);
    }

    bool is_at_end() {
        return index >= source.size();
    }
    
    void add_token(TokenType token_type) {
        tokens.push_back({token_type, source.substr(token_start, index - token_start), line, col, file});
    }

    void error(std::string error_msg) {
        std::cout << "Scanning error at line " << line << " col " << col << ": " << error_msg << std::endl;
        had_error = true;
    }
};