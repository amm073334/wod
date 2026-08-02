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
        // string eventually, but it needs to hold an arbitrary number
        // of operands.
        OPKIND_INTERP,

        // Uses the `local_offset` member; basically a virtual register.
        OPKIND_LOCAL_INT,
        OPKIND_LOCAL_STR,
        OPKIND_TEMP_INT,

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

        size_t local_offset;

        struct {
            StringView path;
            StringView name;
        } global;
    } as;
};

typedef struct {
    enum {
        INST_WIRInst_Binop,
        INST_WIRInst_StrAssign,
        INST_WIRInst_IfBegin,
        INST_WIRInst_LoopBeginN,
        INST_WIRInst_Call,
        INST_WIRInst_ReturnVal,
        INST_WIRInst_Cmd,
        INST_WIRInst_DBLoad,
        INST_WIRInst_DBStore,
        INST_WIRInst_Label,
        INST_WIRInst_Goto,
        INST_WIRInst_Else,
        INST_WIRInst_IfEnd,
        INST_WIRInst_LoopBegin,
        INST_WIRInst_LoopEnd,
        INST_WIRInst_Continue,
        INST_WIRInst_Break,
        INST_WIRInst_ReturnVoid,
    } kind;
} WIRInst;
VEC_PTR_DEF(WIRInst);

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

#endif // WOD_WIR_H_
