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
        _WIRInst_NOP,

        // Used to manipulate the compile-time stack.
        // Does not actually correspond to a real command.
        _WIRInst_PushInt,
        _WIRInst_PushStr,
        _WIRInst_PopIntN,
        _WIRInst_PopStrN,

        // Generally corresponds to a real command.
        _WIRInst_Binop,
        _WIRInst_StrAssign,
        _WIRInst_IfBegin,
        _WIRInst_LoopBeginN,
        _WIRInst_Call,
        _WIRInst_ReturnVal,
        _WIRInst_Cmd,
        _WIRInst_DBLoad,
        _WIRInst_DBStore,
        _WIRInst_Label,
        _WIRInst_Goto,
        _WIRInst_Else,
        _WIRInst_IfEnd,
        _WIRInst_LoopBegin,
        _WIRInst_LoopEnd,
        _WIRInst_Continue,
        _WIRInst_Break,
        _WIRInst_ReturnVoid,
    } kind;
} WIRInst;
VEC_PTR_DEF(WIRInst);

typedef struct {
    WIRInst base;
    size_t n;
} WIRInst_PopIntN;

typedef struct {
    WIRInst base;
    size_t n;
} WIRInst_PopStrN;

typedef struct {
    WIRInst base;
    enum {
        WIR_BINOP_ADD, WIR_BINOP_SUB, WIR_BINOP_MUL, WIR_BINOP_DIV, WIR_BINOP_MOD,
        WIR_BINOP_AND, WIR_BINOP_OR, WIR_BINOP_XOR, WIR_BINOP_LSH,
        WIR_BINOP_EQ, WIR_BINOP_NEQ, WIR_BINOP_LT, WIR_BINOP_LTE,
        WIR_BINOP_GT, WIR_BINOP_GTE,
        WIR_BINOP_LAND, WIR_BINOP_LOR,
    } op;
    enum {
        WIR_ASSIGN_EQ,
        WIR_ASSIGN_ADD,
        WIR_ASSIGN_SUB,
        WIR_ASSIGN_MUL,
        WIR_ASSIGN_DIV,
        WIR_ASSIGN_MOD,
    } assign;
    WIROperand dest;
    WIROperand a;
    WIROperand b;
} WIRInst_Binop;

typedef struct {
    WIRInst base;
} WIRInst_Else;

typedef struct {
    WIRInst base;
} WIRInst_IfEnd;

typedef struct {
    WIRInst base;
} WIRInst_LoopBegin;

typedef struct {
    WIRInst base;
} WIRInst_LoopEnd;

typedef struct {
    WIRInst base;
} WIRInst_Continue;

typedef struct {
    WIRInst base;
} WIRInst_Break;

typedef struct {
    WIRInst base;
} WIRInst_ReturnVoid;

typedef struct {
    WIRInst base;
    WIROperand dest;
    WIROperand src;
} WIRInst_StrAssign;

typedef struct {
    WIRInst base;
    WIROperand cond;
} WIRInst_IfBegin;

typedef struct {
    WIRInst base;
    WIROperand count;
} WIRInst_LoopBeginN;

typedef struct {
    WIRInst base;
    WIROperand src;
    WIROperand db_type;
    WIROperand db_data;
    WIROperand db_field;
} WIRInst_DBLoad;

typedef struct {
    WIRInst base;
    WIROperand dst;
    WIROperand db_type;
    WIROperand db_data;
    WIROperand db_field;
} WIRInst_DBStore;

typedef struct {
    WIRInst base;
    
    // Can be the immediate integer `0` if the
    // return value is unused, or there is no return value.
    WIROperand dest;

    // Either a globally qualified name or a
    // virtual register containing the cev address.
    WIROperand cev;
    
    VEC_WIROperand args;
} WIRInst_Call;

typedef struct {
    WIRInst base;
    WIROperand val;
} WIRInst_ReturnVal;

typedef struct {
    WIRInst base;
    int32_t op;
    VEC_WIROperand iargs;
    VEC_WIROperand sargs;

    // Can be 1 for `open` or -1 for `close`; or 0 for
    // no change. Indicates commands with block-like
    // structures, like loops and conditionals.
    int open_close;
} WIRInst_Cmd;

typedef struct {
    WIRInst base;
    WIROperand name;
} WIRInst_Label;

typedef struct {
    WIRInst base;
    WIROperand name;
} WIRInst_Goto;

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
