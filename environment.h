#pragma once

#include <unordered_map>
#include <string>
#include "common.h"

class Environment;

enum SymType {
    SYM_VAR,
    SYM_CDB,
    SYM_FUNC,
};

struct Symbol {
    SymType st;
    bool is_const = false;
    WodType type;
    int32_t ref = 0; // used as const value if variable is a const int
    std::vector<WodType> arg_types;
    std::string const_string;
    FunctionStmt* inline_function = nullptr;

    Environment* cdb_fields = nullptr;
    Symbol(WodType wt)
        : st(SYM_VAR), type(wt) {}
    Symbol(int32_t i)
        : st(SYM_VAR), is_const(true), type(WodType(TYPE_INT)), ref(i) {}
    Symbol(std::string s)
        : st(SYM_VAR), is_const(true), type(WodType(TYPE_STR)), const_string(s) {}
    Symbol(int32_t ref, WodType wt, std::vector<WodType> args)
        : st(SYM_FUNC), ref(ref), type(wt), arg_types(args) {}
    Symbol(FunctionStmt* fn, WodType wt, std::vector<WodType> arg_types)
        : st(SYM_FUNC), inline_function(fn), type(wt), arg_types(arg_types) {}
    Symbol(Environment* fields)
        : st(SYM_CDB), cdb_fields(fields) {}
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
        return &symbols.insert({name, Symbol(type)}).first->second;
    }

    Symbol* define_const_int(std::string name, int32_t int_val) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, Symbol(int_val)}).first->second;
    }

    Symbol* define_const_str(std::string name, std::string str_val) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, Symbol(str_val)}).first->second;
    }
    
    Symbol* define_function(std::string name, int32_t ref, WodType type, std::vector<WodType> arg_types) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, Symbol(ref, type, arg_types)}).first->second;
    }

    Symbol* define_inline_function(std::string name, FunctionStmt* inline_function, WodType type, std::vector<WodType> arg_types) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, Symbol(inline_function, type, arg_types)}).first->second;
    }

    Symbol* define_cdb(std::string name, Environment* fields) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, Symbol(fields)}).first->second;
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