#ifndef WOD_COMMAND_H_
#define WOD_COMMAND_H_

#include "common.h"

typedef enum {
    CMD_EMPTY        = 0,
    CMD_IF_INT       = 111,
    CMD_IF_STR       = 112,
    CMD_VAR          = 121,
    CMD_STRING       = 122,
    CMD_LOOP         = 170,
    CMD_BREAK        = 171,
    CMD_RETURN       = 172,
    CMD_QUIT_GAME    = 175,
    CMD_CONTINUE     = 176,
    CMD_LOOP_COUNT   = 179,
    CMD_CALL_ID      = 210,
    CMD_CALL_RESERVE = 211,
    CMD_LABEL        = 212,
    CMD_JUMP         = 213,
    CMD_DB           = 250,
    CMD_CALL_NAME    = 300,
    CMD_BRANCH       = 401,
    CMD_BRANCH_ELSE  = 420,
    CMD_LOOP_END     = 498,
    CMD_IF_END       = 499,
} CommandOp;

typedef enum {
    // miscellaneous flags
    VAR_LIMIT           = 0x01,
    VAR_REAL            = 0x02,
    VAR_SUPPRESS_RHS_0  = 0x04,
    VAR_SUPPRESS_RHS_1  = 0x08,
    VAR_DEREF_LHS       = 0x10,
    VAR_DEREF_RHS_0     = 0x20,
    VAR_DEREF_RHS_1     = 0x40,

    // assignment operators
    VAR_ASSIGN_EQ          = 0x000,
    VAR_ASSIGN_PLUS_EQ     = 0x100,
    VAR_ASSIGN_MINUS_EQ    = 0x200,
    VAR_ASSIGN_TIMES_EQ    = 0x300,
    VAR_ASSIGN_DIV_EQ      = 0x400,
    VAR_ASSIGN_MOD_EQ      = 0x500,
    VAR_ASSIGN_LOW_BOUND   = 0x600,
    VAR_ASSIGN_HIGH_BOUND  = 0x700,
    VAR_ASSIGN_ABS         = 0x800,
    VAR_ASSIGN_ATAN        = 0xf900, // for whatever reason, atan needs to have an extra F
    VAR_ASSIGN_SIN         = 0xa00,
    VAR_ASSIGN_COS         = 0xb00,
    VAR_ASSIGN_SQRT        = 0xc00,

    // rhs operators
    VAR_OP_PLUS     = 0x0000,
    VAR_OP_MINUS    = 0x1000,
    VAR_OP_TIMES    = 0x2000,
    VAR_OP_DIV      = 0x3000,
    VAR_OP_MOD      = 0x4000,
    VAR_OP_AND      = 0x5000,
    VAR_OP_RAND     = 0x6000,
    VAR_OP_OR       = 0x7000,
    VAR_OP_XOR      = 0x8000,
    VAR_OP_LSHIFT   = 0x9000,
} CommandVarFlag;

typedef enum {
    CALL_EVAL_NAME      =     0x100,
    CALL_STORES_RETURN  = 0x1000000,
} CallFlag;

typedef struct {
    CommandOp id;
    VEC_int32_t int_list;
    VEC_StringView str_list;
    uint8_t indent;
} Command;

#endif // WOD_COMMAND_H_