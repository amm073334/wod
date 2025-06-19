#pragma once

#include <cstdint>

const int32_t CEV_THRESHOLD = 500000;
const int32_t VAR_THRESHOLD = 1000000;
const int32_t CSELF_THRESHOLD = 1600000;
const int32_t MAX_CEV_REF = CEV_THRESHOLD + 9999;
const int32_t MAX_CSELF_REF = CSELF_THRESHOLD + 99;

enum WodType {
    TYPE_VOID,
    TYPE_INT,
    TYPE_STR,
    TYPE_INTPTR
};