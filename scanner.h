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
            token_start = pos;
            scan_token();
        }

        tokens.push_back(Token(T_EOF, "", line));
        return tokens;
    }

    bool failed() { return had_error; }

private:
    std::vector<Token> tokens;
    std::string source;
    size_t token_start = 0;
    size_t pos = 0;
    size_t line = 1;
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
        char c = source.at(pos++);
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
                if (match('/')) while (peek() != '\n' && !is_at_end()) pos++;
                else add_token(T_SLASH);
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n': line++; break;
            default:
                if (std::isdigit(c)) try_number();
                else if (is_alpha_under(c)) try_identifier();
                else error();
                break;
        }
    }

    void try_number() {
        while (std::isdigit(peek())) pos++;
        add_token(T_NUMBER, std::stoi(source.substr(token_start, pos - token_start)));
    }

    void try_identifier() {
        while (is_alpha_under(peek()) || std::isdigit(peek())) pos++;
        std::string text = source.substr(token_start, pos - token_start);
        if (keywords.count(text)) {
            add_token(keywords.at(text));
        } else {
            add_token(T_IDENT, text);
        }
    }

    // utility
    bool is_alpha_under(char c) {
        return std::isalpha(c) || c == '_';
    }

    bool match(char expected) {
        if (is_at_end()) return false;
        if (source.at(pos) != expected) return false;
        pos++;
        return true;
    }

    char peek() {
        if (is_at_end()) return '\0';
        return source.at(pos);
    }

    bool is_at_end() {
        return pos >= source.size();
    }
    
    void add_token(TokenType token_type, std::string s) {
        tokens.push_back(Token(token_type, source.substr(token_start, pos - token_start), s, line));
    }

    void add_token(TokenType token_type, int32_t n) {
        tokens.push_back(Token(token_type, source.substr(token_start, pos - token_start), n, line));
    }

    void add_token(TokenType token_type) {
        tokens.push_back(Token(token_type, source.substr(token_start, pos - token_start), line));
    }

    void error() {
        std::cout << "Scanning error at line " << line << ": " << source.at(pos) << std::endl;
        had_error = true;
    }
};