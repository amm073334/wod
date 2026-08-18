#ifndef WOD_LIR_H_
#define WOD_LIR_H_

#include "common.h"

typedef struct {
    enum {
        // Integer immediates are assumed to be less than
        // the reference conversion threshold.
        LIR_I_IMM,
        LIR_I_REF_INT,
        LIR_I_REF_STR,
    } kind;
    int32_t value;
} LIROperand;
VEC_DEF(LIROperand);

typedef struct {
    enum {
        LIR_S_IMM,
        LIR_S_INTERP,
    } kind;

} LIRString;

typedef struct {
    enum {
        LIR_BINOP,
        LIR_STR,
        LIR_IFINT,
        LIR_BRANCH,
        LIR_IFEND,
        LIR_LOOP,
        LIR_LOOPN,
        LIR_LOOPEND,
        LIR_CONTINUE,
        LIR_BREAK,
        LIR_CALL,
        LIR_CMD,
        LIR_DBLOAD,
        LIR_DBSTORE,
        LIR_LABEL,
        LIR_GOTO,
        LIR_RETURN,
    } kind;
    VEC_LIROperand i_fields;
    VEC_StringView s_fields;
} LIRInst;
VEC_DEF(LIRInst);

typedef struct {
    VEC_LIRInst insts;
    size_t ret;
} LIR;

#endif // WOD_LIR_H_