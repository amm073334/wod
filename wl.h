#ifndef WOD_WL_H_
#define WOD_WL_H_

#include "common.h"
#include "commonevent.h"

typedef enum {
    WL_VAR, // (dest, a, b, op)
    
    WL_INT_IF_HEAD,
    WL_INT_CMP,

    WL_STR_IF_HEAD,
    WL_STR_CMP,

    WL_ELSE,
    WL_END_IF,

    WL_LOOP,
    WL_LOOP_N, // (n)
    WL_LOOP_END,

    // (src/dst, type, data, field)
    WL_DB_LOAD, WL_DB_STORE,

    // (cev, dest, iargs..., sargs...)
    WL_CALL,

    WL_RETURN,

    // (op, indent, iargs..., sargs...)
    WL_CMD,

    WL_LABEL, WL_GOTO, // (name)
} WLOpcode;

typedef enum {
    OPERAND_NONE,
    
    // Uses the integer member.
    OPERAND_INT,

    // Uses the vself member; basically a virtual register.
    OPERAND_LOCAL,
    OPERAND_TMP,

    // Uses string operand.
    OPERAND_STR,
} WLOperandType;

typedef struct {
    WLOperandType type;
    union {
        int32_t integer;
        size_t vself;
        StringView string;
    } as;
} WLOperand;

VEC_DEF(WLOperand);

typedef struct {
    WLOpcode op;
    VEC_WLOperand operands;
} WLInst;

VEC_DEF(WLInst);

CommonEvent compile_wl_to_cev(VEC_WLInst *arr, Arena *arena);

#endif // WOD_WL_H_