#include "wir.h"

#define I_BASE 1600010
#define S_BASE 1600005

static uint8_t indent = 0;

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

CommonEvent compile_wir_to_cev(WIR wir, Arena *arena) {

    CommonEvent cev;
    cev_init(&cev, arena);

    for (size_t i = 0; i < wir.cevs.count; i++) {
        VEC_int32_t i_vec;
        VEC_StringView s_vec;
    
        // WIROperand *operands = wir.cevs.at[i];
        // switch (arr->at[i].op) {
        //     case WIR_VAR: {
        //         assert(operands[0].kind == OPKIND_LOCAL ||
        //                operands[0].kind == OPKIND_TMP);
            
        //         // set_last_i_vself(operands[0].as.vself);
            
        //         int32_t dest = operands[0].as.vself + I_BASE;

        //         VEC_INIT(i_vec);

        //         int32_t a = disable_rc(arena, &cev, operands[1]);
        //         int32_t b = disable_rc(arena, &cev, operands[2]);

        //         VEC_PUSH(i_vec, operands[0].as.vself, arena);
        //         VEC_PUSH(i_vec, a, arena);
        //         VEC_PUSH(i_vec, b, arena);
        //         VEC_PUSH(i_vec, operands[3].as.integer, arena);
                
        //         VEC_INIT(s_vec);

        //         cev_push_cmd(&cev, CMD_VAR, indent, i_vec, s_vec);
        //         break;
        //     }
        //     // TODO
        //     case WIR_INT_IF_HEAD:
        //         cev_push_simple_cmd(&cev, indent, CMD_IF_INT);
        //         break;

        //     case WIR_LOOP_BEGIN:
        //         cev_push_simple_cmd(&cev, indent, CMD_LOOP);
        //         break;
        //     case WIR_LOOP_BEGIN_N: {
        //         VEC_INIT(i_vec);
        //         int32_t n = disable_rc(arena, &cev, operands[0]);
        //         VEC_PUSH(i_vec, n, arena);
                
        //         VEC_INIT(s_vec);

        //         cev_push_cmd(&cev, indent, CMD_LOOP, i_vec, s_vec);
        //         break;
        //     }
        //     case WIR_LOOP_END:
        //         cev_push_simple_cmd(&cev, indent, CMD_LOOP_END);
        //         break;
        //     default:
        //         UNREACHABLE;
        // }
    }
    return cev;
}

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
                    printf(" " SV_FMT, SV_FMT_VAL(operand.as.imm_str));
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

    for (size_t i = 0; i < wir->dbs.count; i++) {
        WIRDB *db = &wir->dbs.at[i];

        printf("(DB) " SV_FMT ":\n", SV_FMT_VAL(db->debug_name));
        for (int j = 0; j < db->fields.count; j++) {
            WIRDBField *field = &db->fields.at[j];
            
            printf(SV_FMT "\n", SV_FMT_VAL(field->debug_name));
        }
    }

}
