#pragma once

#include <string>

enum Type {
    TYPE_INT,
    TYPE_STR
};

struct LitVal {
    LitVal() = default;
    LitVal(int32_t n) : n(n) {}
    LitVal(std::string s) : s(s) {}
    int32_t n;
    std::string s;
};