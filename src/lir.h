#ifndef WOD_LIR_H_
#define WOD_LIR_H_

#include "common.h"

typedef struct {
    enum {
        // Integer immediates are assumed to be less than
        // the reference conversion threshold.
        LIR_IMM_INT,
        LIR_IMM_STR,
        LIR_REF_INT,
        LIR_REF_STR,
    } kind;
    union {
        int32_t imm_int;
        StringView imm_str;
        size_t offset;
    } as;
} LIROperand;

typedef struct {
    enum {
        _LIRInst_Binop,
        _LIRInst_StrAssign,
        _LIRInst_IfBegin,
        _LIRInst_LoopBeginN,
        _LIRInst_Call,
        _LIRInst_ReturnVal,
        _LIRInst_Cmd,
        _LIRInst_DBLoad,
        _LIRInst_DBStore,
        _LIRInst_Label,
        _LIRInst_Goto,
        _LIRInst_Else,
        _LIRInst_IfEnd,
        _LIRInst_LoopBegin,
        _LIRInst_LoopEnd,
        _LIRInst_Continue,
        _LIRInst_Break,
        _LIRInst_ReturnVoid,
    } kind;
} LIRInst;

#endif // WOD_LIR_H_