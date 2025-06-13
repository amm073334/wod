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

        tokens.push_back({T_EOF, "", line, col});
        return tokens;
    }

    bool failed() { return had_error; }

private:
    std::vector<Token> tokens;
    std::string source;
    size_t token_start = 0;
    size_t index = 0;
    size_t line = 1;
    size_t col = 1;
    bool had_error = false;

    const std::unordered_map<std::string, TokenType> keywords = {
        {"void", T_VOID},
        {"int", T_INT},
        {"str", T_STR},
        {"return", T_RETURN},
        {"if", T_IF},
        {"else", T_ELSE},
        {"loop", T_LOOP}
    };

    void scan_token() {
        char c = advance();
        switch (c) {
            case '{': add_token(T_LBRACE); break;
            case '}': add_token(T_RBRACE); break;
            case '(': add_token(T_LPAREN); break;
            case ')': add_token(T_RPAREN); break;
            case ',': add_token(T_COMMA); break;
            case '.': add_token(T_DOT); break;
            case '+': add_token(T_PLUS); break;
            case '-': add_token(T_MINUS); break;
            case '*': add_token(T_STAR); break;
            case '%': add_token(T_PERCENT); break;
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
                else add_token(T_SLASH);
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n': line++; col = 1; break;
            case '"': try_string(); break;
            default:
                if (std::isdigit(c)) try_number();
                else if (is_alpha_under(c)) try_identifier();
                else error("Unexpected character");
                break;
        }
    }

    void try_number() {
        while (std::isdigit(peek())) advance();
        add_token(T_NUMBER);
    }

    void try_identifier() {
        while (is_alpha_under(peek()) || std::isdigit(peek())) advance();
        std::string text = source.substr(token_start, index - token_start);
        if (keywords.count(text)) {
            add_token(keywords.at(text));
        } else {
            add_token(T_IDENT);
        }
    }

    void try_string() {
        while (peek() != '"' && !is_at_end()) {
            advance();
        }
        if (is_at_end() || peek() == '\n') {
            error("Unterminated string");
        }
        tokens.push_back({T_STRING, source.substr(token_start + 1, index - token_start), line, col});
        advance();
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
        tokens.push_back({token_type, source.substr(token_start, index - token_start), line, col});
    }

    void error(std::string error_msg) {
        std::cout << "Scanning error at line " << line << ": " << error_msg << std::endl;
        had_error = true;
    }
};