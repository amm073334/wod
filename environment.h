#pragma once

#include <unordered_map>
#include <string>
#include "common.h"

class Environment;

struct Symbol {
    bool is_const = false;
    WodType type;
    int32_t ref = 0; // used as const value if variable is a const int
    std::vector<WodType> arg_types;
    std::string const_string;
    FunctionStmt* inline_function = nullptr;

    Environment* cdb_fields = nullptr;
};

class Environment {
public:
    Environment()
        : enclosing(nullptr) {}
    Environment(Environment* enclosing)
        : enclosing(enclosing) { enclosing->children.push_back(this); }
    ~Environment() { for (Environment* child : children) delete child; }

    Symbol* define(std::string name, WodType type) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, {false, type}}).first->second;
    }

    Symbol* define_const_int(std::string name, int32_t int_val) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, {true, WodType(TYPE_INT), int_val}}).first->second;
    }

    Symbol* define_const_str(std::string name, std::string str_val) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, {true, WodType(TYPE_STR), 0, {}, str_val}}).first->second;
    }
    
    Symbol* define_function(std::string name, int32_t ref, WodType type, std::vector<WodType> arg_types) {
        if (symbols.count(name)) return nullptr;
        Symbol s;
        s.type = type;
        s.ref = ref;
        s.arg_types = arg_types;
        return &symbols.insert({name, {false, type, ref, arg_types}}).first->second;
    }

    Symbol* define_inline_function(std::string name, FunctionStmt* inline_function, WodType type, std::vector<WodType> arg_types) {
        if (symbols.count(name)) return nullptr;
        Symbol s;
        s.type = type;
        s.arg_types = arg_types;
        s.inline_function = inline_function;
        return &symbols.insert({name, {false, type, 0, arg_types, "", inline_function}}).first->second;
    }

    Symbol* define_cdb(std::string name, Environment* fields) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, {false, WodType(TYPE_VOID), 0, {}, "", nullptr, fields}}).first->second;
    }

    Symbol* get(std::string name) {
        if (symbols.count(name)) return &symbols.at(name);
        if (enclosing != nullptr) return enclosing->get(name);
        return nullptr;
    }

    Environment* parent() {
        return enclosing;
    }

    void print() {
        std::cout << "{ ";
        for (auto p : symbols) {
            std::cout << p.first << " ";
        }
        std::cout << std::endl;
        for (Environment* c : children) {
            c->print();
        }
        std::cout << "}" << std::endl;
    }

private:
    Environment* enclosing;
    std::list<Environment*> children;
    std::unordered_map<std::string, Symbol> symbols;
};