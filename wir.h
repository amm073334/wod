#ifndef WOD_WIR_H_
#define WOD_WIR_H_

#include "common.h"
#include "commonevent.h"
#include "environment.h"
#include "gamedata.h"

#define WIR_IMM_I(i) (WIROperand){ .kind = OPKIND_IMM, .type = OPTYPE_INT, .as.imm_int = (i) }
#define WIR_IMM_S(s) (WIROperand){ .kind = OPKIND_IMM, .type = OPTYPE_STR, .as.imm_str = (s) }

typedef struct WIROperand {
    enum {
        // Immediate.
        OPKIND_IMM,

        // Uses the `imm_str` field to qualify a globally-scoped identifier.
        // For example, `example/file.wod:global_name`.
        OPKIND_GLOBAL,

        // Uses the offset member; basically a virtual register.
        OPKIND_LOCAL,
        OPKIND_TMP,
    } kind;

    // Type of the operand (not of the union). For example,
    // an integer global would still use the `imm_str` field.
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
        WIR_IF_BEGIN,

        WIR_ELSE, WIR_IF_END,

        WIR_LOOP_BEGIN,
        WIR_LOOP_BEGIN_N, // (n)
        WIR_LOOP_END,

        // (src/dst, type, data, field)
        WIR_DB_LOAD, WIR_DB_STORE,

        // (dest, cev, iargs..., sargs...)
        // `cev` is either a globally qualified name
        //    (like `directory/file.wod:cev_name`)
        // or a virtual register containing the cev address.
        WIR_CALL,

        WIR_CONTINUE, WIR_BREAK,
        WIR_RETURN_VOID,
        
        // (src)
        WIR_RETURN_VAL,

        // (op, open/close, iargs..., sargs...)
        // `open/close` can be 1 for `open` or -1 for `close`;
        // or 0 for no change. Indicates commands with block-like
        // structures, like loops and conditionals.
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
} WIRCev;

VEC_DEF(WIRCev);

typedef struct {
    StringView debug_name;
    WodType type;

    bool has_initializer;
    WIROperand initializer;
} WIRDBField;

VEC_DEF(WIRDBField);

typedef struct {
    StringView debug_name;
    VEC_WIRDBField fields;
} WIRDB;

VEC_DEF(WIRDB);

typedef struct {
    VEC_WIRCev cevs;
    VEC_WIRDB udb_types;
    VEC_WIRDB cdb_types;
} WIR;

GameData compile_wir_to_gd(WIR arr, Arena *arena);
void print_wir(WIR *wir);

#endif // WOD_WIR_H_
