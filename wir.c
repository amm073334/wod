#include "wir.h"

#define I_BASE 1600010
#define S_BASE 1600005

typedef struct TableEntry {
    StringView path;
    StringView name;
    int32_t value;
} TableEntry;
VEC_DEF(TableEntry);

typedef struct WIRCompiler {
    // Maps globally-qualified names to global addresses.
    VEC_TableEntry table;

    GameData gd;
    Arena *arena;
} WIRCompiler;

// TODO: It seems like disabling rc isn't really viable at the wir layer,
//       because allocating space for temporaries requires knowledge
//       of the state of the compile-time stack (which means it should
//       be done during AST traversal).
//
//       That said, the wir layer is still necessary for distinguishing
//       between locals and temporaries, and potentially for handling
//       if-else chains.
static int32_t disable_rc(Arena *arena, CommonEvent *cev, WIROperand wop) {
    (void) arena;
    (void) cev;
    (void) wop;
    // if (wop.type != OPERAND_INT) return wop.as.integer;
    // if (wop.as.integer < 1000000) return wop.as.integer;
    
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

int32_t resolve_top_level(WIRCompiler *wc, WIROperand wop) {
    assert(wop.kind == OPKIND_GLOBAL);

    for (size_t i = 0; i < wc->table.count; i++) {
        TableEntry *e = &wc->table.at[i];
        if (sv_equals(e->path, wop.as.top.path)
            && sv_equals(e->name, wop.as.top.name))
            return e->value;
    }

    UNREACHABLE;
    return 0;
}

#define CEV_BINOP(op) do { \
        assert(wi->operands.at[0].kind == OPKIND_LOCAL \
                || wi->operands.at[0].kind == OPKIND_TMP); \
 \
        int32_t dest = wi->operands.at[0].as.offset + I_BASE; \
        int32_t a = disable_rc(wc->arena, &cev, wi->operands.at[1]); \
        int32_t b = disable_rc(wc->arena, &cev, wi->operands.at[2]); \
 \
        VEC_PUSH(i_vec, dest, wc->arena); \
        VEC_PUSH(i_vec, a, wc->arena); \
        VEC_PUSH(i_vec, b, wc->arena); \
        VEC_PUSH(i_vec, op, wc->arena); \
         \
        cev_push_cmd(&cev, CMD_VAR, indent, i_vec, s_vec); \
    } while (0)

void compile_wir(WIRCompiler *wc, WIR *wir) {
    for (size_t i = 0; i < wir->cevs.count; i++) {
        CommonEvent cev;
        cev_init(&cev, wc->arena);

        WIRCev *wcev = &wir->cevs.at[i];

        // TODO: This is a bandaid to specify the entry point,
        //       but if WIR is to be a file-local concept then it doesn't
        //       make sense to specify a global entry point here.
        if (sv_equals(wcev->debug_name, SV("main"))) {
            wc->gd.entry = 500000 + i;
        }

        uint8_t indent = 0;
        for (size_t j = 0; j < wcev->insts.count; j++) {
            VEC_int32_t i_vec = VEC_EMPTY;
            VEC_StringView s_vec = VEC_EMPTY;
            
            WIRInst *wi = &wcev->insts.at[j];
            switch (wi->op) {
            case WIR_ADD: CEV_BINOP(VAR_OP_PLUS); break;
            case WIR_SUB: CEV_BINOP(VAR_OP_MINUS); break;
            case WIR_MUL: CEV_BINOP(VAR_OP_TIMES); break;
            case WIR_DIV: CEV_BINOP(VAR_OP_DIV); break;
            case WIR_MOD: CEV_BINOP(VAR_OP_MOD); break;
            case WIR_XOR: CEV_BINOP(VAR_OP_XOR); break;
            case WIR_LSH: CEV_BINOP(VAR_OP_LSHIFT); break;
            case WIR_AND: CEV_BINOP(VAR_OP_AND); break;
            case WIR_OR:  CEV_BINOP(VAR_OP_OR); break;

            case WIR_EQ: UNIMPLEMENTED;
            case WIR_NEQ: UNIMPLEMENTED;
            case WIR_LT: UNIMPLEMENTED;
            case WIR_LTE: UNIMPLEMENTED;
            case WIR_GT: UNIMPLEMENTED;
            case WIR_GTE: UNIMPLEMENTED;
            case WIR_LAND: UNIMPLEMENTED;
            case WIR_LOR: UNIMPLEMENTED;
            // TODO
            case WIR_IF_BEGIN:
                cev_push_simple_cmd(&cev, CMD_IF_INT, indent);
                
                indent++;
                break;

            case WIR_LOOP_BEGIN:
                cev_push_simple_cmd(&cev, CMD_LOOP, indent);
                
                indent++;
                break;
            case WIR_LOOP_BEGIN_N: {
                VEC_INIT(i_vec);
                int32_t n = disable_rc(wc->arena, &cev, wi->operands.at[0]);
                VEC_PUSH(i_vec, n, wc->arena);
                
                VEC_INIT(s_vec);

                cev_push_cmd(&cev, CMD_LOOP_COUNT, indent, i_vec, s_vec);
                
                indent++;
                break;
            }
            case WIR_LOOP_END:
                indent--;

                cev_push_simple_cmd(&cev, CMD_LOOP_END, indent);
                break;
            case WIR_RETURN_VAL:
                cev_push_simple_cmd(&cev, CMD_RETURN, indent);
                break;
            case WIR_CMD: {
                assert(wi->operands.count >= 2);
                assert(wi->operands.at[0].kind == OPKIND_IMM);
                assert(wi->operands.at[1].kind == OPKIND_IMM);
                assert(wi->operands.at[1].as.imm_int >= -1
                       && wi->operands.at[1].as.imm_int <= 1);

                size_t str_op_pos;
                {
                    size_t o = 2;
                    while (o < wi->operands.count
                           && wi->operands.at[o].type == OPTYPE_INT)
                        o++;
                    
                    str_op_pos = o;
                }

                VEC_int32_t int_fields = VEC_EMPTY;
                for (size_t o = 2; o < str_op_pos; o++) {
                    VEC_PUSH(int_fields, wi->operands.at[o].as.imm_int, wc->arena);
                }

                VEC_StringView str_fields = VEC_EMPTY;
                for (size_t o = str_op_pos; o < wi->operands.count; o++) {
                    assert(wi->operands.at[o].type == OPTYPE_STR);
                    VEC_PUSH(str_fields, wi->operands.at[o].as.imm_str, wc->arena);
                }

                if (wi->operands.at[1].as.imm_int == -1)
                    indent--;

                cev_push_cmd(&cev,
                    wi->operands.at[0].as.imm_int,
                    indent,
                    int_fields, str_fields);

                if (wi->operands.at[1].as.imm_int == 1)
                    indent++;
                
                break;
            }

            case WIR_CALL: {

                // cev_push_cmd(&cev, CMD_CALL_ID, indent, )
            }
            default:
                UNIMPLEMENTED;
        }
        }
        assert(indent == 0);

        cev_push_cmd(&cev, 0, indent,
            (VEC_int32_t)VEC_EMPTY, (VEC_StringView)VEC_EMPTY);

        VEC_PUSH(wc->gd.cevs, cev, wc->arena);
    }

    for (size_t i = 0; i < wir->cdb_types.count; i++) {

    }
}

GameData wir_pass(VEC_Module *modules, Arena *arena) {
    WIRCompiler wc = {
        .table = VEC_EMPTY,
        .arena = arena
    };
    gd_init(&wc.gd);
    
    // Iterate backwards. Assuming that the modules list is
    // a topological sort of the import graph, this should
    // register every top-level symbol's offset before its use.
    for (size_t i = 0; i < modules->count; i++) {
        compile_wir(&wc, modules->at[modules->count - i - 1].wir);
    }

    DB db;
    db_init(&db);
    VEC_PUSH(wc.gd.cdb, db, arena);

    return wc.gd;
}

#undef CEV_BINOP

void print_wir(WIR *wir) {

    for (size_t cev = 0; cev < wir->cevs.count; cev++) {
        VEC_WIRInst *arr = &wir->cevs.at[cev].insts;

        printf(SV_FMT ":\n", SV_FMT_VAL(wir->cevs.at[cev].debug_name));

        for (size_t i = 0; i < arr->count; i++) {
            WIRInst inst = arr->at[i];
            printf("[OP %d]:", inst.op);
            for (size_t j = 0; j < inst.operands.count; j++) {
                WIROperand operand = inst.operands.at[j];
                switch (operand.kind) {
                case OPKIND_IMM:
                    if (operand.type == OPTYPE_INT)
                        printf(" %d", operand.as.imm_int);
                    else 
                        printf(" \"" SV_FMT "\"", SV_FMT_VAL(operand.as.imm_str));
                    break;
                case OPKIND_GLOBAL:
                    printf(" " SV_FMT ":" SV_FMT,
                        SV_FMT_VAL(operand.as.top.path),
                        SV_FMT_VAL(operand.as.top.name));
                    break;
                case OPKIND_LOCAL:
                    printf(" $L%zu", operand.as.offset);
                    break;
                case OPKIND_TMP:
                    printf(" $T%zu", operand.as.offset);
                    break;
                }
            }
            printf("\n");
        }
        printf("\n");
    }

    for (size_t i = 0; i < wir->cdb_types.count; i++) {
        WIRDB *db = &wir->cdb_types.at[i];

        printf("(DB) " SV_FMT ":\n", SV_FMT_VAL(db->debug_name));
        for (size_t j = 0; j < db->fields.count; j++) {
            WIRDBField *field = &db->fields.at[j];
            
            printf(SV_FMT "\n", SV_FMT_VAL(field->debug_name));
        }
    }

}
