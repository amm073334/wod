#pragma once

#include <cstdint>

const int32_t CEV_THRESHOLD = 500000;
const int32_t VAR_THRESHOLD = 1000000;
const int32_t CSELF_THRESHOLD = 1600000;
const int32_t MAX_CEV_REF = CEV_THRESHOLD + 9999;
const int32_t MAX_CSELF_REF = CSELF_THRESHOLD + 99;

struct WodType {
    enum Type {
        TYPE_VOID,
        TYPE_INT,
        TYPE_STR,
        TYPE_INTPTR,
        TYPE_CDB,
    } t = TYPE_VOID;
    int32_t db_type = 0;

    friend bool operator==(const WodType& l, const WodType& r) {
        if (l.t == TYPE_CDB && r.t == TYPE_CDB) return l.db_type == r.db_type;
        else return l.t == r.t;
    }
    friend bool operator==(const WodType& l, const Type& r) { return l.t == r; }
    friend bool operator!=(const WodType& l, const WodType& r) { return !(l == r); }
    friend bool operator!=(const WodType& l, const Type& r) { return !(l == r); }
};