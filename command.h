#pragma once

#include <cstdint>

enum CommandId : int32_t {
    CMD_EMPTY           = 0,
    CMD_IF_INT          = 111,
    CMD_IF_STR          = 112,
    CMD_ARITH           = 121,
    CMD_STRING          = 122,
    CMD_LOOP            = 170,
    CMD_BREAK           = 171,
    CMD_RETURN          = 172,
    CMD_CONTINUE        = 176,
    CMD_LOOP_COUNT      = 179,
    CMD_CALL_ID         = 210,
    CMD_CALL_RESERVE    = 211,
    CMD_LABEL           = 212,
    CMD_JUMP            = 213,
    CMD_DB              = 250,
    CMD_CALL_NAME       = 300,
    CMD_BRANCH          = 401,
    CMD_BRANCH_ELSE     = 420,
    CMD_LOOP_END        = 498,
    CMD_IF_END          = 499,
};

enum IfFlag : int32_t {
    IF_HAS_ELSE = 0x10,
};

enum IfIntBranchFlag : int32_t {
    IF_INT_OP_GT,
    IF_INT_OP_GTE,
    IF_INT_OP_EQ,
    IF_INT_OP_LTE,
    IF_INT_OP_LT,
    IF_INT_OP_NEQ,
    IF_INT_OP_AND,
    IF_INT_BRANCH_SUPPRESS = 0x10
};

enum IfStrBranchFlag : int32_t {
    IF_STR_OP_EQ            = 0x00000000,
    IF_STR_OP_NEQ           = 0x10000000,
    IF_STR_OP_CONTAINS      = 0x20000000,
    IF_STR_OP_STARTSWITH    = 0x30000000,
    IF_STR_BRANCH_REF       =  0x1000000
};

enum ArithFlag : int32_t {
    // miscellaneous flags
    ARITH_LIMIT           = 0x01,
    ARITH_REAL            = 0x02,
    ARITH_SUPPRESS_RHS_0  = 0x04,
    ARITH_SUPPRESS_RHS_1  = 0x08,
    ARITH_DEREF_LHS       = 0x10,
    ARITH_DEREF_RHS_0     = 0x20,
    ARITH_DEREF_RHS_1     = 0x40,

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
    ARITH_ASSIGN_ATAN        = 0xf900, // for whatever reason, atan needs to have an extra F
    ARITH_ASSIGN_SIN         = 0xa00,
    ARITH_ASSIGN_COS         = 0xb00,
    ARITH_ASSIGN_SQRT        = 0xc00,

    // rhs operators
    ARITH_OP_PLUS     = 0x0000,
    ARITH_OP_MINUS    = 0x1000,
    ARITH_OP_TIMES    = 0x2000,
    ARITH_OP_DIV      = 0x3000,
    ARITH_OP_MOD      = 0x4000,
    ARITH_OP_AND      = 0x5000,
    ARITH_OP_RAND     = 0x6000,
    ARITH_OP_OR       = 0x7000,
    ARITH_OP_XOR      = 0x8000,
    ARITH_OP_LSHIFT   = 0x9000,
};

enum StringFlag {
    STRING_RHS_LIT          =    0x0,
    STRING_RHS_REF          =    0x1,
    STRING_RHS_KB           =    0x2,
    STRING_RHS_DEREF        =    0x3,
    STRING_DEREF_LHS        =   0x10,
    STRING_KB_CANCELABLE    = 0x1000,
    STRING_KB_INITIALIZE    = 0x2000,

    STRING_ASSIGN_EQ        =   0x0,
    STRING_ASSIGN_PLUS_EQ   = 0x100,
};

enum DBFlag {
    DB_DEREF    = 0x1,
    DB_STRLIT   = 0x2,

    DB_ASSIGN_EQ            = 0x00,
    DB_ASSIGN_PLUS_EQ       = 0x10,
    DB_ASSIGN_MINUS_EQ      = 0x20,
    DB_ASSIGN_TIMES_EQ      = 0x30,
    DB_ASSIGN_DIV_EQ        = 0x40,
    DB_ASSIGN_MOD_EQ        = 0x50,
    DB_ASSIGN_LOW_BOUND     = 0x60,
    DB_ASSIGN_HIGH_BOUND    = 0x70,

    DB_TYPE_CDB = 0x000,
    DB_TYPE_SDB = 0x100,
    DB_TYPE_UDB = 0x200,

    DB_ASSIGN_TO_VAR = 0x1000,

    DB_STR_TYPE = 0x10000,
    DB_STR_DATA = 0x20000,
    DB_STR_PROP = 0x40000,
};

enum CallFlag {
    CALL_EVAL_NAME      =     0x100,
    CALL_STORES_RETURN  = 0x1000000,
};

struct Command {
    int32_t command_id;
    int8_t indent_level = 0;
    std::vector<int32_t> int_fields;
    std::vector<std::string> str_fields;
};