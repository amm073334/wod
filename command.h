#pragma once

#include <cstdint>

enum CommandId : int32_t {
    CMD_EMPTY = 0,
    CMD_ARITH = 121,
    CMD_RETURN = 172,
    CMD_CALLNAME = 300
};

enum ArithFlags : int32_t {
    // miscellaneous flags
    ARITH_FLAG_LIMIT           = 0x01,
    ARITH_FLAG_REAL            = 0x02,
    ARITH_FLAG_SUPPRESS_RHS_0  = 0x04,
    ARITH_FLAG_SUPPRESS_RHS_1  = 0x08,
    ARITH_FLAG_DEREF_LHS       = 0x10,
    ARITH_FLAG_DEREF_RHS_0     = 0x20,
    ARITH_FLAG_DEREF_RHS_1     = 0x40,

    // assignment operators
    ARITH_ASSIGN_EQ          = 0x000,
    ARITH_ASSIGN_PLUS_EQ     = 0x100,
    ARITH_ASSIGN_MINUS_EQ    = 0x200,
    ARITH_ASSIGN_TIMES_EQ    = 0x300,
    ARITH_ASSIGN_DIV_EQ      = 0x400,
    ARITH_ASSIGN_MOD_EQ      = 0x500,
    ARITH_ASSIGN_LOW_BOUND   = 0x600,
    ARITH_ASSIGN_HIGH_BOUND  = 0x700,
    ARITH_ASSIGN_ABS         = 0x800,
    ARITH_ASSIGN_ATAN        = 0xf900, // for whatever reason, atan seems to have an extra F
    ARITH_ASSIGN_SIN         = 0xa00,
    ARITH_ASSIGN_COS         = 0xb00,
    ARITH_ASSIGN_SQRT        = 0xc00,

    // rhs operators
    ARITH_OP_PLUS     = 0x0000,
    ARITH_OP_MINUS    = 0x1000,
    ARITH_OP_TIMES    = 0x2000,
    ARITH_OP_DIV      = 0x3000,
    ARITH_OP_MOD      = 0x4000,
    ARITH_OP_BITAND   = 0x5000
};

struct Command {
    int32_t command_id;
    int8_t indent_level = 0;
    std::vector<int32_t> int_fields;
    std::vector<std::string> str_fields;
};