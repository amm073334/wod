#include "wir.h"

#define I_BASE 1600010
#define S_BASE 1600005

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
    case OPKIND_IMM_INT:
        return false;
    case OPKIND_IMM_STR:
        return true;
    case OPKIND_LOCAL:
    case OPKIND_TEMP:
        return wop.as.local.type == LOCAL_STR;
    case OPKIND_GLOBAL:
        return wop.as.global.type == GLOBAL_STR;
    }

    UNREACHABLE;
    return false;
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

int32_t resolve(WIRCompiler *wc, WIROperand wop) {
    assert(wop.kind != OPKIND_IMM_STR);
    
    switch (wop.kind) {
    case OPKIND_IMM_INT: {
        return disable_rc(wc->arena, wc->cev, wop.as.imm_int);
    }
    case OPKIND_LOCAL:
    case OPKIND_TEMP: {
        switch (wop.as.local.type) {
        case LOCAL_INT: return wop.as.local.offset + I_BASE;
        case LOCAL_STR: return wop.as.local.offset + S_BASE;
        }
    }
    case OPKIND_GLOBAL: {
        VEC_GlobalEntry *vec = NULL;
        size_t offset = 0;
        switch (wop.as.global.type) {
        case GLOBAL_INT: vec = &wc->g_ints; offset = 2000000; break;
        case GLOBAL_STR: vec = &wc->g_strs; offset = 3000000; break;
        case GLOBAL_CEV: vec = &wc->g_cevs; offset = 500000; break;
        case GLOBAL_UDB: vec = &wc->g_udbs; offset = 0; break;
        case GLOBAL_CDB: vec = &wc->g_cdbs; offset = 0; break;
        }
    
        for (size_t i = 0; i < vec->count; i++) {
            if (sv_equals(wop.as.global.path, vec->at[i].path)
                && sv_equals(wop.as.global.name, vec->at[i].name)) {
                    
                return offset + i;
            }
        }
        UNREACHABLE;
        return 0;
    }
    case OPKIND_IMM_STR: UNREACHABLE;
    }

    return 0;
}

static void binop(WIRCompiler *wc, WIRInst_Binop *inst, int cmd_var_op) {
    VEC_int32_t i_vec = VEC_EMPTY;
    
    int32_t dest = resolve(wc, inst->dest);
    int32_t a = resolve(wc, inst->a);
    int32_t b = resolve(wc, inst->b);

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
    // TODO
    case INST_WIRInst_IfBegin:
        cev_push_simple_cmd(wc->cev, CMD_IF_INT, wc->indent);
        
        wc->indent++;
        break;

    case INST_WIRInst_LoopBegin:
        cev_push_simple_cmd(wc->cev, CMD_LOOP, wc->indent);
        
        wc->indent++;
        break;
    case INST_WIRInst_LoopBeginN: {
        WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wi;

        VEC_int32_t i_vec = VEC_EMPTY;
        int32_t n = resolve(wc, inst->count);
        
        VEC_PUSH(i_vec, n, wc->arena);
        
        cev_push_cmd(wc->cev, CMD_LOOP_COUNT, wc->indent,
            i_vec, (VEC_StringView)VEC_EMPTY);
        
        wc->indent++;
        break;
    }
    case INST_WIRInst_LoopEnd:
        wc->indent--;

        cev_push_simple_cmd(wc->cev, CMD_LOOP_END, wc->indent);
        break;
    case INST_WIRInst_ReturnVal:
        cev_push_simple_cmd(wc->cev, CMD_RETURN, wc->indent);
        break;
    case INST_WIRInst_Cmd: {
        WIRInst_Cmd *inst = (WIRInst_Cmd *)wi;
        assert(inst->open_close >= -1
                && inst->open_close <= 1);

        VEC_int32_t int_fields = VEC_EMPTY;
        for (size_t arg = 0; arg < inst->iargs.count; arg++) {
            VEC_PUSH(int_fields, inst->iargs.at[arg].as.imm_int, wc->arena);
        }

        VEC_StringView str_fields = VEC_EMPTY;
        for (size_t arg = 0; arg < inst->sargs.count; arg++) {
            assert(inst->sargs.at[arg].kind == OPKIND_IMM_STR);
            VEC_PUSH(str_fields, inst->sargs.at[arg].as.imm_str, wc->arena);
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

        int32_t cev = resolve(wc, inst->cev);

        VEC_int32_t int_args = VEC_EMPTY;
        VEC_int32_t str_ref_args = VEC_EMPTY;
        VEC_StringView str_lit_args = VEC_EMPTY;

        int32_t total_int_args = 0;
        int32_t total_str_args = 0;
        int32_t strlit_flags = 0;
        for (size_t i = 0; i < inst->args.count; i++) {
            WIROperand wop = inst->args.at[i];
            if (is_string(wop)) {
                if (wop.kind == OPKIND_IMM_STR) {
                    VEC_PUSH(str_ref_args, 0, wc->arena);
                    VEC_PUSH(str_lit_args, wop.as.imm_str, wc->arena);
                    strlit_flags |= (1 << total_str_args);    
                } else {
                    VEC_PUSH(str_lit_args, SV(""), wc->arena);
                }
                total_str_args++;
            } else {
                VEC_PUSH(int_args, resolve(wc, wop), wc->arena);
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
            VEC_PUSH(int_fields, resolve(wc, inst->dest), wc->arena);

        cev_push_cmd(wc->cev,
            CMD_CALL_ID, wc->indent,
            int_fields,
            strlit_flags ? str_lit_args : (VEC_StringView)VEC_EMPTY
        );
        break;
    }
    default:
        UNIMPLEMENTED;
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

        // TODO: This is a bandaid to specify the entry point,
        //       but if WIR is to be a file-local concept then it doesn't
        //       make sense to specify a global entry point here.
        if (sv_equals(wcev->name, SV("main"))) {
            wc->gd.entry = 500000 + i;
        }

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
    
    // Iterate backwards. Assuming that the modules list is
    // a topological sort of the import graph, this should
    // register every top-level symbol's offset before its use.
    for (size_t i = 0; i < modules->count; i++) {
        compile_wir(&wc, &modules->at[modules->count - i - 1]);
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

void print_wir(WIR *wir) {

    for (size_t cev = 0; cev < wir->g_cevs.count; cev++) {
        VEC_PTR_WIRInst arr = wir->g_cevs.at[cev].insts;

        printf(SV_FMT ":\n", SV_FMT_VAL(wir->g_cevs.at[cev].name));

        for (size_t i = 0; i < arr.count; i++) {
            WIRInst *inst = arr.at[i];
            printf("[OP %d]:", inst->kind);
            // for (size_t j = 0; j < inst.operands.count; j++) {
            //     WIROperand operand = inst.operands.at[j];
            //     switch (operand.kind) {
            //     case OPKIND_IMM:
            //         if (operand.type == OPTYPE_INT)
            //             printf(" %d", operand.as.imm_int);
            //         else 
            //             printf(" \"" SV_FMT "\"", SV_FMT_VAL(operand.as.imm_str));
            //         break;
            //     case OPKIND_GLOBAL:
            //         printf(" " SV_FMT ":" SV_FMT,
            //             SV_FMT_VAL(operand.as.top.path),
            //             SV_FMT_VAL(operand.as.top.name));
            //         break;
            //     case OPKIND_LOCAL:
            //         printf(" $L%zu", operand.as.offset);
            //         break;
            //     case OPKIND_TMP:
            //         printf(" $T%zu", operand.as.offset);
            //         break;
            //     }
            // }
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
