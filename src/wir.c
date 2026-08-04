#include "wir.h"

#define CSELF_BASE 1600000
#define CSELF_INT_BASE 1600010
#define CSELF_INT_MAX  1600099
#define CSELF_STR_BASE 1600005
#define CSELF_STR_MAX  1600009

#define NORMAL_VAR_BASE 2000000
#define STRING_VAR_BASE 3000000
#define CEV_BASE 500000
#define UDB_BASE 10000000
#define CDB_BASE 11000000

typedef struct GlobalEntry {
    StringView path;
    StringView name;
} GlobalEntry;
VEC_DEF(GlobalEntry);

typedef struct WIRCompiler {
    // Maps globally-qualified names to global indices.
    VEC_GlobalEntry g_cevs;
    VEC_GlobalEntry g_cdbs;
    VEC_GlobalEntry g_udbs;
    VEC_GlobalEntry g_ints;
    VEC_GlobalEntry g_strs;

    CommonEvent *cev;
    uint8_t indent;

    GameData gd;
    Arena *arena;
} WIRCompiler;

bool is_string(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_LOCAL_STR:
        case OPKIND_GLOBAL_STR:
            return true;
        case OPKIND_IMM_INT:
        case OPKIND_TEMP_INT:
        case OPKIND_LOCAL_INT:
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            return false;
    }

    UNREACHABLE;
    return false;
}

bool is_strlit(WIROperand wop) {
    return wop.kind == OPKIND_IMM_STR
        || wop.kind == OPKIND_INTERP;
}

// TODO: It seems like disabling rc isn't really viable at the wir layer,
//       because allocating space for temporaries requires knowledge
//       of the state of the compile-time stack (which means it should
//       be done during AST traversal).
//
//       That said, the wir layer is still necessary for distinguishing
//       between locals and temporaries, and potentially for handling
//       if-else chains.
static int32_t disable_rc(Arena *arena, CommonEvent *cev, int32_t imm) {
    (void) arena;
    (void) cev;
    if (imm < 1000000) return imm;
    
    // int32_t temp = push_i_temp();

    // VEC_int32_t i_vec;
    // VEC_StringView s_vec;
    // VEC_INIT(i_vec);
    // VEC_PUSH(i_vec, temp, arena);
    // VEC_PUSH(i_vec, 0, arena);
    // VEC_PUSH(i_vec, -wop.as.integer, arena);
    // VEC_PUSH(i_vec, VAR_OP_MINUS, arena);

    // VEC_INIT(s_vec);

    // cev_push_cmd(cev, CMD_VAR, indent, i_vec, s_vec);

    // return temp;
    return 0;
}

// If `yobidasi` is true, returns the 呼び出し値.
// Otherwise, returns the index (for example, CSelf index).
int32_t resolve(WIRCompiler *wc, WIROperand wop, bool yobidasi) {
    assert(wop.kind != OPKIND_IMM_STR && wop.kind != OPKIND_INTERP);
    
    VEC_GlobalEntry *g_vec = NULL;
    size_t offset = 0;

    switch (wop.kind) {
    case OPKIND_IMM_INT: {
        return wop.as.imm_int;
    }
    case OPKIND_LOCAL_INT:
    case OPKIND_LOCAL_STR:
    case OPKIND_TEMP_INT: {
        // TODO: Fix temp offsets.
        int32_t ref;
        if (wop.kind == OPKIND_LOCAL_STR) {
            ref = wop.as.local_offset + CSELF_STR_BASE;
            assert(ref <= CSELF_STR_MAX);
        } else {
            ref = wop.as.local_offset + CSELF_INT_BASE;
            assert(ref <= CSELF_INT_MAX);
        }
        return yobidasi ? ref : ref - CSELF_BASE;
    }
    case OPKIND_GLOBAL_INT: g_vec = &wc->g_ints; offset = NORMAL_VAR_BASE; break;
    case OPKIND_GLOBAL_STR: g_vec = &wc->g_strs; offset = STRING_VAR_BASE; break;
    case OPKIND_GLOBAL_CEV: g_vec = &wc->g_cevs; offset = CEV_BASE; break;
    case OPKIND_GLOBAL_UDB: g_vec = &wc->g_udbs; offset = UDB_BASE; break;
    case OPKIND_GLOBAL_CDB: g_vec = &wc->g_cdbs; offset = CDB_BASE; break;
    
    case OPKIND_IMM_STR:
    case OPKIND_INTERP:
        UNREACHABLE;
    }

    for (size_t i = 0; i < g_vec->count; i++) {
        if (sv_equals(wop.as.global.path, g_vec->at[i].path)
            && sv_equals(wop.as.global.name, g_vec->at[i].name)) {
                
            return yobidasi ? offset + i : i;
        }
    }

    UNREACHABLE;
    return 0;
}

StringView interpolate(WIRCompiler *wc, WIROperand wop) {
    assert(is_strlit(wop));

    if (wop.kind == OPKIND_IMM_STR)
        return wop.as.imm_str;

    assert(wop.kind == OPKIND_INTERP);

    StringView out = SV("");
    for (size_t i = 0; i < wop.as.interp.count; i++) {
        WIROperand w = wop.as.interp.at[i];

        char buf[sizeof(int32_t) * 8 + 1];
        switch (w.kind) {
        case OPKIND_IMM_STR:
            out = sv_concat(wc->arena, out, w.as.imm_str);
            break;
        case OPKIND_INTERP:
            out = sv_concat(wc->arena, out, interpolate(wc, w));
            break;
        case OPKIND_IMM_INT: {
            snprintf(buf, sizeof(buf), "%d", w.as.imm_int);
            out = sv_concat(wc->arena, out, to_sv(buf));
            break;
        }
        case OPKIND_LOCAL_INT:
        case OPKIND_LOCAL_STR:
        case OPKIND_TEMP_INT: {
            // TODO: Maybe the string-processing stuff needs an upgrade.
            snprintf(buf, sizeof(buf), "\\cself[%d]", resolve(wc, w, false));
            out = sv_concat(wc->arena, out, to_sv(buf));
            break;
        }
        case OPKIND_GLOBAL_INT: {
            snprintf(buf, sizeof(buf), "\\v[%d]", resolve(wc, w, false));
            out = sv_concat(wc->arena, out, to_sv(buf));
            break;
        }
        case OPKIND_GLOBAL_STR: {
            snprintf(buf, sizeof(buf), "\\s[%d]", resolve(wc, w, false));
            out = sv_concat(wc->arena, out, to_sv(buf));
            break;
        }
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            out = sv_concat(wc->arena, out, w.as.global.name);
            break;
        }
    }

    return out;
}

static void binop(WIRCompiler *wc, WIRInst_Binop *inst, int cmd_var_op) {
    VEC_int32_t i_vec = VEC_EMPTY;
    
    int32_t dest = resolve(wc, inst->dest, true);
    int32_t a = resolve(wc, inst->a, true);
    int32_t b = resolve(wc, inst->b, true);

    VEC_PUSH(i_vec, dest, wc->arena);
    VEC_PUSH(i_vec, a, wc->arena);
    VEC_PUSH(i_vec, b, wc->arena);
    VEC_PUSH(i_vec, cmd_var_op, wc->arena);

    cev_push_cmd(wc->cev, CMD_VAR, wc->indent, i_vec, (VEC_StringView)VEC_EMPTY);
}

static void compile_inst(WIRCompiler *wc, WIRInst *wi) {
    switch (wi->kind) {
    case INST_WIRInst_Binop: {
        WIRInst_Binop *inst = (WIRInst_Binop *)wi;
        switch (inst->op) {
        case WIR_ADD: binop(wc, inst, VAR_OP_PLUS); break;
        case WIR_SUB: binop(wc, inst, VAR_OP_MINUS); break;
        case WIR_MUL: binop(wc, inst, VAR_OP_TIMES); break;
        case WIR_DIV: binop(wc, inst, VAR_OP_DIV); break;
        case WIR_MOD: binop(wc, inst, VAR_OP_MOD); break;
        case WIR_XOR: binop(wc, inst, VAR_OP_XOR); break;
        case WIR_LSH: binop(wc, inst, VAR_OP_LSHIFT); break;
        case WIR_AND: binop(wc, inst, VAR_OP_AND); break;
        case WIR_OR:  binop(wc, inst, VAR_OP_OR); break;
        
        case WIR_EQ: UNIMPLEMENTED;
        case WIR_NEQ: UNIMPLEMENTED;
        case WIR_LT: UNIMPLEMENTED;
        case WIR_LTE: UNIMPLEMENTED;
        case WIR_GT: UNIMPLEMENTED;
        case WIR_GTE: UNIMPLEMENTED;
        case WIR_LAND: UNIMPLEMENTED;
        case WIR_LOR: UNIMPLEMENTED;
        }
        break;                
    }
    case INST_WIRInst_StrAssign: {
        WIRInst_StrAssign *inst = (WIRInst_StrAssign *)wi;

        VEC_int32_t int_fields = VEC_EMPTY;
        VEC_StringView str_fields = VEC_EMPTY;
        if (inst->src.kind == OPKIND_IMM_STR) {
            VEC_PUSH(int_fields, resolve(wc, inst->dest, true), wc->arena);
            VEC_PUSH(int_fields, 0, wc->arena);
            VEC_PUSH(int_fields, 0, wc->arena);
            VEC_PUSH(str_fields, inst->src.as.imm_str, wc->arena);
        } else {
            VEC_PUSH(int_fields, resolve(wc, inst->dest, true), wc->arena);
            VEC_PUSH(int_fields, 0, wc->arena);
            VEC_PUSH(int_fields, resolve(wc, inst->src, true), wc->arena);
        }
        
        cev_push_cmd(wc->cev, CMD_STRING, wc->indent,
            int_fields, str_fields);
        break;
    }
    // TODO
    case INST_WIRInst_IfBegin: {
        cev_push_simple_cmd(wc->cev, CMD_IF_INT, wc->indent);
        
        wc->indent++;
        break;
    }
    case INST_WIRInst_LoopBegin: {
        cev_push_simple_cmd(wc->cev, CMD_LOOP, wc->indent);
        
        wc->indent++;
        break;
    }
    case INST_WIRInst_LoopBeginN: {
        WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wi;

        VEC_int32_t i_vec = VEC_EMPTY;
        int32_t n = resolve(wc, inst->count, true);
        
        VEC_PUSH(i_vec, n, wc->arena);
        
        cev_push_cmd(wc->cev, CMD_LOOP_COUNT, wc->indent,
            i_vec, (VEC_StringView)VEC_EMPTY);
        
        wc->indent++;
        break;
    }
    case INST_WIRInst_LoopEnd: {
        wc->indent--;

        cev_push_simple_cmd(wc->cev, CMD_LOOP_END, wc->indent);
        break;
    }
    case INST_WIRInst_ReturnVal: {
        cev_push_simple_cmd(wc->cev, CMD_RETURN, wc->indent);
        break;
    }
    case INST_WIRInst_Cmd: {
        WIRInst_Cmd *inst = (WIRInst_Cmd *)wi;
        assert(inst->open_close >= -1
                && inst->open_close <= 1);

        VEC_int32_t int_fields = VEC_EMPTY;
        for (size_t i = 0; i < inst->iargs.count; i++) {
            VEC_PUSH(int_fields, resolve(wc, inst->iargs.at[i], true), wc->arena);
        }

        VEC_StringView str_fields = VEC_EMPTY;
        for (size_t i = 0; i < inst->sargs.count; i++) {
            WIROperand wop = inst->sargs.at[i];
            if (is_strlit(wop))
                VEC_PUSH(str_fields, interpolate(wc, wop), wc->arena);
            else UNREACHABLE;
        }

        if (inst->open_close == -1)
            wc->indent--;

        cev_push_cmd(wc->cev,
            inst->op,
            wc->indent,
            int_fields, str_fields);

        if (inst->open_close == 1)
            wc->indent++;
        
        break;
    }
    case INST_WIRInst_Call: {
        WIRInst_Call *inst = (WIRInst_Call *)wi;

        int32_t cev = resolve(wc, inst->cev, true);

        VEC_int32_t int_args = VEC_EMPTY;
        VEC_int32_t str_ref_args = VEC_EMPTY;
        VEC_StringView str_lit_args = VEC_EMPTY;
        VEC_PUSH(str_lit_args, SV(""), wc->arena);

        int32_t total_int_args = 0;
        int32_t total_str_args = 0;
        int32_t strlit_flags = 0;
        for (size_t i = 0; i < inst->args.count; i++) {
            WIROperand wop = inst->args.at[i];
            if (is_string(wop)) {
                if (is_strlit(wop)) {
                    VEC_PUSH(str_ref_args, 0, wc->arena);
                    VEC_PUSH(str_lit_args, interpolate(wc, wop), wc->arena);
                    strlit_flags |= (1 << total_str_args);
                } else {
                    VEC_PUSH(str_ref_args, resolve(wc, wop, true), wc->arena);
                    VEC_PUSH(str_lit_args, SV(""), wc->arena);
                }
                total_str_args++;
            } else {
                VEC_PUSH(int_args, resolve(wc, wop, true), wc->arena);
                total_int_args++;
            }
        }

        // TODO: This can be expanded later.
        assert(total_int_args <= 5 && total_str_args <= 5);

        int32_t flags =
            total_int_args
            | total_str_args << 4
            | strlit_flags << 12;

        if (inst->dest.kind == OPKIND_IMM_INT
            && inst->dest.as.imm_int == 0)
            flags |= CALL_STORES_RETURN;

        VEC_int32_t int_fields = VEC_EMPTY;
        VEC_PUSH(int_fields, cev, wc->arena);
        VEC_PUSH(int_fields, flags, wc->arena);

        for (size_t i = 0; i < int_args.count; i++)
            VEC_PUSH(int_fields, int_args.at[i], wc->arena);
        
        for (size_t i = 0; i < str_ref_args.count; i++)
            VEC_PUSH(int_fields, str_ref_args.at[i], wc->arena);

        if (flags & CALL_STORES_RETURN)
            VEC_PUSH(int_fields, resolve(wc, inst->dest, true), wc->arena);

        cev_push_cmd(wc->cev,
            CMD_CALL_ID, wc->indent,
            int_fields,
            strlit_flags ? str_lit_args : (VEC_StringView)VEC_EMPTY
        );
        break;
    }
    case INST_TOMBSTONE: break;
    default:
        UNIMPLEMENTED;
    }        
}

// If the temporary result of a calculation is moved directly into a
// variable, rewrite to store the result directly into the variable.
static void temp_copy_propagation(WIRCev *wcev) {
    for (size_t i = 0; i < wcev->insts.count; i++) {
        if (wcev->insts.at[i]->kind != INST_WIRInst_Binop)
            continue;
        
        WIRInst_Binop *inst = (WIRInst_Binop *)wcev->insts.at[i]; 

        if (inst->op != WIR_ADD) continue;
        if (inst->a.kind != OPKIND_TEMP_INT) continue;
        if (inst->b.kind != OPKIND_IMM_INT) continue;
        if (inst->b.as.imm_int != 0) continue;

        assert(i > 0);
        WIRInst *prev = wcev->insts.at[i - 1];

        if (prev->kind == INST_WIRInst_Binop) {
            WIRInst_Binop *p = (WIRInst_Binop *)prev;
            if (p->dest.kind == OPKIND_TEMP_INT
                && p->dest.as.local_offset == inst->a.as.local_offset) {

                p->dest = inst->dest;
                inst->base.kind = INST_TOMBSTONE;
            }
        }
    }
}


static void compile_wir(WIRCompiler *wc, Module *mod) {
    WIR *wir = mod->wir;

    // Dump all the top-level symbols in the current file
    // into the global list.
    {
        #define LOAD_SYMBOLS(dst, src) do { \
            for (size_t i = 0; i < src.count; i++) { \
                VEC_PUSH(dst, ((GlobalEntry){ \
                    .path = mod->source->path, \
                    .name = src.at[i].name \
                }), wc->arena); \
            }} while (0)
        
        LOAD_SYMBOLS(wc->g_ints, wir->g_ints);
        LOAD_SYMBOLS(wc->g_strs, wir->g_strs);
        LOAD_SYMBOLS(wc->g_cevs, wir->g_cevs);
        LOAD_SYMBOLS(wc->g_udbs, wir->g_udbs);
        LOAD_SYMBOLS(wc->g_cdbs, wir->g_cdbs);
        
        #undef LOAD_SYMBOLS
    }
    
    // Then compile all the common events.
    for (size_t i = 0; i < wir->g_cevs.count; i++) {
        CommonEvent cev;
        cev_init(&cev, wc->arena);

        wc->cev = &cev;
        wc->indent = 0;

        WIRCev *wcev = &wir->g_cevs.at[i];

        temp_copy_propagation(wcev);

        for (size_t j = 0; j < wcev->insts.count; j++) {
            compile_inst(wc, wcev->insts.at[j]);
        }
        assert(wc->indent == 0);
    
        cev_push_cmd(&cev, 0, wc->indent,
            (VEC_int32_t)VEC_EMPTY, (VEC_StringView)VEC_EMPTY);

        VEC_PUSH(wc->gd.cevs, cev, wc->arena);
    }
}

GameData wir_pass(VEC_Module *modules, Arena *arena) {
    WIRCompiler wc = {
        .g_ints = VEC_EMPTY,
        .g_strs = VEC_EMPTY,
        .g_cevs = VEC_EMPTY,
        .g_udbs = VEC_EMPTY,
        .g_cdbs = VEC_EMPTY,
        .arena = arena
    };
    gd_init(&wc.gd);
    
    for (size_t i = 0; i < modules->count; i++) {
        compile_wir(&wc, &modules->at[i]);
    }

    // Assign entry point.
    for (size_t i = 0; i < wc.g_cevs.count; i++) {
        if (sv_equals(wc.g_cevs.at[i].name, SV("main"))) {
            wc.gd.entry = 500000 + i;
        }
    }

    DB db;
    db_init(&db);
    VEC_PUSH(wc.gd.cdb, db, arena);

    return wc.gd;
}

void wir_init(WIR *wir) {
    VEC_INIT(wir->g_ints);
    VEC_INIT(wir->g_strs);
    VEC_INIT(wir->g_cevs);
    VEC_INIT(wir->g_udbs);
    VEC_INIT(wir->g_cdbs);
}

static void print_wop(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_INT:
            printf(" %d", wop.as.imm_int);
            return;
        case OPKIND_IMM_STR:
            printf(" \"" SV_FMT "\"", SV_FMT_VAL(wop.as.imm_str));
            return;
        case OPKIND_INTERP:
            printf(" ${");
            for (size_t i = 0; i < wop.as.interp.count; i++) {
                print_wop(wop.as.interp.at[i]);
            }
            printf("}");
            return;
        case OPKIND_LOCAL_INT:
            printf(" $LI(%zu)", wop.as.local_offset);
            return;
        case OPKIND_LOCAL_STR:
            printf(" $LS(%zu)", wop.as.local_offset);
            return;
        case OPKIND_TEMP_INT:
            printf(" $T(%zu)", wop.as.local_offset);
            return;
        case OPKIND_GLOBAL_INT:
            printf(" $GINT[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_STR:
            printf(" $GSTR[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_CEV:
            printf(" $GCEV[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_UDB:
            printf(" $GUDB[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_CDB:
            printf(" $GCDB[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
    }
}

void print_wir(WIR *wir) {
    for (size_t cev = 0; cev < wir->g_cevs.count; cev++) {
        VEC_PTR_WIRInst arr = wir->g_cevs.at[cev].insts;

        printf(SV_FMT ":\n", SV_FMT_VAL(wir->g_cevs.at[cev].name));

        for (size_t i = 0; i < arr.count; i++) {
            WIRInst *inst = arr.at[i];
            printf("[OP %d]:", inst->kind);
            switch (inst->kind) {
            case INST_WIRInst_Binop: {
                WIRInst_Binop *in = (WIRInst_Binop *)inst;
                printf(" %d", in->op);
                print_wop(in->dest);
                print_wop(in->a);
                print_wop(in->b);
                break;
            }
            case INST_TOMBSTONE:
                printf(" X");
                break;
            default:
                printf(" (print not implemented)");
                break;
            }
            printf("\n");
        }
        printf("\n");
    }

    for (size_t i = 0; i < wir->g_cdbs.count; i++) {
        WIRDB *db = &wir->g_cdbs.at[i];

        printf("(DB) " SV_FMT ":\n", SV_FMT_VAL(db->name));
        for (size_t j = 0; j < db->fields.count; j++) {
            WIRVar *field = &db->fields.at[j];
            
            printf(SV_FMT "\n", SV_FMT_VAL(field->name));
        }
    }
}

bool op_is_local(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_INT:
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_TEMP_INT:
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_STR:
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            return false;
        case OPKIND_LOCAL_INT:
        case OPKIND_LOCAL_STR:
            return true;
    }
    UNREACHABLE;
    return false;
}

bool op_is_global(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_INT:
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_TEMP_INT:
        case OPKIND_LOCAL_INT:
        case OPKIND_LOCAL_STR:
            return false;
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_STR:
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            return true;
    }
    UNREACHABLE;
    return false;
}