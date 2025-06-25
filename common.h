#pragma once

#include <cstdint>

const int32_t CEV_THRESHOLD = 500000;
const int32_t VAR_THRESHOLD = 1000000;
const int32_t CSELF_THRESHOLD = 1600000;
const int32_t MAX_CEV_REF = CEV_THRESHOLD + 9999;
const int32_t MAX_CSELF_REF = CSELF_THRESHOLD + 99;

enum Basetype {
    TYPE_VOID,
    TYPE_INT,
    TYPE_STR,
    TYPE_INTARR,
};

struct WodType {
    Basetype ty = TYPE_VOID;
    int32_t i = 0;
    WodType() = default;
    WodType(Basetype ty) : ty(ty) {}
    WodType(Basetype ty, int32_t i) : ty(ty), i(i) {}
    friend bool operator==(const WodType& l, const WodType& r) {
        if (l.ty == TYPE_INTARR && r.ty == TYPE_INTARR) return l.i == r.i; 
        return l.ty == r.ty;
    }
    friend bool operator==(const WodType& l, const Basetype& r) { return l.ty == r; }
    friend bool operator!=(const WodType& l, const WodType& r) { return !(l == r); }
    friend bool operator!=(const WodType& l, const Basetype& r) { return !(l == r); }
};