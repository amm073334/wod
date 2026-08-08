#ifndef WOD_WIR_H_
#define WOD_WIR_H_

#include "common.h"
#include "commonevent.h"
#include "environment.h"
#include "gamedata.h"
#include "module.h"

#define WIR_IMM_I(i) (WIROperand){ .kind = OPKIND_IMM_INT, .as.imm_int = (i) }
#define WIR_IMM_S(s) (WIROperand){ .kind = OPKIND_IMM_STR, .as.imm_str = (s) }

typedef struct WIROperand WIROperand;
VEC_DEF(WIROperand);
struct WIROperand {
    enum {
        // Immediate.
        OPKIND_IMM_INT,
        OPKIND_IMM_STR,

        // Interpolated string. This gets compiled down to a single
        // native string eventually, but it needs to hold an arbitrary
        // number of operands here.
        OPKIND_INTERP,

        // Uses the `offset` member.
        //
        // For locals, the offset is relative to the function base
        // (in other words, CSelf0). This mimics the behavior of
        // locals on the stack of a typical programming language.
        //
        // For temporaries, the offset is like a virtual register,
        // and is never meant to decrease over the course of a
        // function. Instead, temporaries are allocated in the
        // CSelf space such that they don't overlap with the locals.
        OPKIND_LOCAL_INT,
        OPKIND_LOCAL_STR,
        OPKIND_TEMP_INT,
        OPKIND_TEMP_STR,

        // Uses the `global` field to qualify a top-level identifier.
        // Integer variables are assumed to index into 通常変数.
        OPKIND_GLOBAL_INT,
        OPKIND_GLOBAL_STR,
        OPKIND_GLOBAL_CEV,
        OPKIND_GLOBAL_UDB,
        OPKIND_GLOBAL_CDB,
    } kind;

    union {
        int32_t imm_int;
        StringView imm_str;

        VEC_WIROperand interp;

        size_t offset;

        struct {
            StringView path;
            StringView name;
        } global;
    } as;
};

typedef struct {
    enum {
        // No-op.
        E_INST_TOMBSTONE,

        // Used to manipulate the compile-time stack.
        // Does not actually correspond to a real command.
        E_INST_PushInt,
        E_INST_PushStr,
        E_INST_PopIntN,
        E_INST_PopStrN,

        // Generally corresponds to a real command.
        E_INST_Binop,
        E_INST_StrAssign,
        E_INST_IfBegin,
        E_INST_LoopBeginN,
        E_INST_Call,
        E_INST_ReturnVal,
        E_INST_Cmd,
        E_INST_DBLoad,
        E_INST_DBStore,
        E_INST_Label,
        E_INST_Goto,
        E_INST_Else,
        E_INST_IfEnd,
        E_INST_LoopBegin,
        E_INST_LoopEnd,
        E_INST_Continue,
        E_INST_Break,
        E_INST_ReturnVoid,
    } kind;
} WIRInst;
VEC_PTR_DEF(WIRInst);

typedef struct {
    WIRInst base;
    size_t n;
} INST_PopIntN;

typedef struct {
    WIRInst base;
    size_t n;
} INST_PopStrN;

typedef struct {
    WIRInst base;
    enum {
        WIR_ADD, WIR_SUB, WIR_MUL, WIR_DIV, WIR_MOD,
        WIR_AND, WIR_OR, WIR_XOR, WIR_LSH,
        WIR_EQ, WIR_NEQ, WIR_LT, WIR_LTE,
        WIR_GT, WIR_GTE,
        WIR_LAND, WIR_LOR,
    } op;
    WIROperand dest;
    WIROperand a;
    WIROperand b;
} INST_Binop;

typedef struct {
    WIRInst base;
} INST_Else;

typedef struct {
    WIRInst base;
} INST_IfEnd;

typedef struct {
    WIRInst base;
} INST_LoopBegin;

typedef struct {
    WIRInst base;
} INST_LoopEnd;

typedef struct {
    WIRInst base;
} INST_Continue;

typedef struct {
    WIRInst base;
} INST_Break;

typedef struct {
    WIRInst base;
} INST_ReturnVoid;

typedef struct {
    WIRInst base;
    WIROperand dest;
    WIROperand src;
} INST_StrAssign;

typedef struct {
    WIRInst base;
    WIROperand cond;
} INST_IfBegin;

typedef struct {
    WIRInst base;
    WIROperand count;
} INST_LoopBeginN;

typedef struct {
    WIRInst base;
    WIROperand src;
    WIROperand db_type;
    WIROperand db_data;
    WIROperand db_field;
} INST_DBLoad;

typedef struct {
    WIRInst base;
    WIROperand dst;
    WIROperand db_type;
    WIROperand db_data;
    WIROperand db_field;
} INST_DBStore;

typedef struct {
    WIRInst base;
    
    // Can be the immediate integer `0` if the
    // return value is unused, or there is no return value.
    WIROperand dest;

    // Either a globally qualified name or a
    // virtual register containing the cev address.
    WIROperand cev;
    
    VEC_WIROperand args;
} INST_Call;

typedef struct {
    WIRInst base;
    WIROperand val;
} INST_ReturnVal;

typedef struct {
    WIRInst base;
    int32_t op;
    VEC_WIROperand iargs;
    VEC_WIROperand sargs;

    // Can be 1 for `open` or -1 for `close`; or 0 for
    // no change. Indicates commands with block-like
    // structures, like loops and conditionals.
    int open_close;
} INST_Cmd;

typedef struct {
    WIRInst base;
    WIROperand name;
} INST_Label;

typedef struct {
    WIRInst base;
    WIROperand name;
} INST_Goto;

typedef struct WIRCev {
    StringView name;
    VEC_PTR_WIRInst insts;

    // Convenience fields to keep track of the lowest unused
    // virtual temporary.
    size_t n_temp_ints;
    size_t n_temp_strs;
} WIRCev;
VEC_DEF(WIRCev);

typedef struct WIRVar {
    StringView name;
    WodType type;

    bool has_initializer;
    WIROperand initializer;
} WIRVar;
VEC_DEF(WIRVar);

typedef struct WIRDB {
    StringView name;
    VEC_WIRVar fields;
} WIRDB;
VEC_DEF(WIRDB);

typedef struct WIR {
    // Each of these fields is a list of global (top-level)
    // symbols in the file, separated by kind.
    VEC_WIRVar g_ints;
    VEC_WIRVar g_strs;
    VEC_WIRCev g_cevs;
    VEC_WIRDB g_udbs;
    VEC_WIRDB g_cdbs;
} WIR;

GameData wir_pass(VEC_Module *modules, Arena *arena);
void wir_init(WIR *wir);
void print_wir(WIR *wir);

bool op_is_local(WIROperand wop);
bool op_is_global(WIROperand wop);
bool op_is_string(WIROperand wop);

#endif // WOD_WIR_H_
