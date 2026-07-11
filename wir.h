#ifndef WOD_WIR_H_
#define WOD_WIR_H_

#include "common.h"
#include "commonevent.h"

#define WIR_IMM_I(i) (WIROperand){ .kind = OPKIND_IMM, .type = OPTYPE_INT, .as.imm_int = (i) }
#define WIR_IMM_S(s) (WIROperand){ .kind = OPKIND_IMM, .type = OPTYPE_STR, .as.imm_str = (s) }

typedef struct WIROperand {
    enum {
        // Immediate.
        OPKIND_IMM,

        // Uses the offset member; basically a virtual register.
        OPKIND_GLOBAL,
        OPKIND_LOCAL,
        OPKIND_TMP,
    } kind;

    enum {
        OPTYPE_INT,
        OPTYPE_STR,
    } type;

    union {
        int32_t imm_int;
        StringView imm_str;
        size_t offset;
    } as;
} WIROperand;

VEC_DEF(WIROperand);

typedef struct {
    enum {
        // For handling scopes.
        WIR_PUSH_LOCAL, WIR_POP_LOCAL,

        // (dest, a, b)
        WIR_ADD, WIR_SUB, WIR_MUL, WIR_DIV, WIR_MOD,
        WIR_AND, WIR_OR, WIR_XOR, WIR_LSH,
        WIR_EQ, WIR_NEQ, WIR_LT, WIR_LTE,
        WIR_GT, WIR_GTE,
        WIR_LAND, WIR_LOR,

        // (dest, src)
        WIR_STR_ASSIGN,

        // (cond)
        WIR_INT_IF_HEAD, WIR_STR_IF_HEAD, WIR_BRANCH,
        
        WIR_ELSE, WIR_IF_END,

        WIR_LOOP,
        WIR_LOOP_N, // (n)
        WIR_LOOP_END,

        // (src/dst, type, data, field)
        WIR_DB_LOAD, WIR_DB_STORE,

        // (dest, cev, iargs..., sargs...)
        WIR_CALL,

        WIR_CONTINUE, WIR_BREAK,
        WIR_RETURN_VOID,
        
        // (src)
        WIR_RETURN_VAL,

        // (op, indent, iargs..., sargs...)
        WIR_CMD,

        // (name)
        WIR_LABEL, WIR_GOTO,
    } op;
    VEC_WIROperand operands;
} WIRInst;

VEC_DEF(WIRInst);

typedef struct {
    StringView debug_name;
    VEC_WIRInst insts;
} WIRFunc;

VEC_DEF(WIRFunc);

typedef struct {
    VEC_WIRFunc cevs;
} WIR;

CommonEvent compile_wir_to_cev(VEC_WIRInst *arr, Arena *arena);
void print_wir(WIR *wir);

#endif // WOD_WIR_H_
