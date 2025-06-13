#pragma once

#include <unordered_map>
#include <string>
#include "common.h"

struct Symbol {
    WodType type;
    int32_t ref;
    std::vector<WodType> arg_types;
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
        return &symbols.insert({name, Symbol({type})}).first->second;
    }
    
    Symbol* define_function(std::string name, int32_t ref, WodType type, std::vector<WodType> arg_types) {
        if (symbols.count(name)) return nullptr;
        return &symbols.insert({name, Symbol({type, ref, arg_types})}).first->second;
    }

    Symbol* get(std::string name) {
        if (symbols.count(name)) return &symbols.at(name);
        if (enclosing != nullptr) return enclosing->get(name);
        return nullptr;
    }

    Environment* parent() {
        return enclosing;
    }

private:
    Environment* enclosing;
    std::vector<Environment*> children;
    std::unordered_map<std::string, Symbol> symbols;
};