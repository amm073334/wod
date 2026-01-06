#include "wl.h"

#define I_BASE 1600010
#define S_BASE 1600005

static uint8_t indent = 0;

// TODO: It seems like disabling rc isn't really viable at the wl layer,
//       because allocating space for temporaries requires knowledge
//       of the state of the compile-time stack (which means it should
//       be done during AST traversal).
//
//       That said, the wl layer is still necessary for distinguishing
//       between locals and temporaries, and potentially for handling
//       if-else chains.
static int32_t disable_rc(Arena *arena, CommonEvent *cev, WLOperand wlop) {
    (void) arena;
    (void) cev;
    (void) wlop;
    // if (wlop.type != OPERAND_INT) return wlop.as.integer;
    // if (wlop.as.integer < 1000000) return wlop.as.integer;
    
    // int32_t temp = push_i_temp();

    // VEC_int32_t i_vec;
    // VEC_StringView s_vec;
    // VEC_INIT(i_vec);
    // VEC_PUSH(i_vec, temp, arena);
    // VEC_PUSH(i_vec, 0, arena);
    // VEC_PUSH(i_vec, -wlop.as.integer, arena);
    // VEC_PUSH(i_vec, VAR_OP_MINUS, arena);

    // VEC_INIT(s_vec);

    // cev_push_cmd(cev, CMD_VAR, indent, i_vec, s_vec);

    // return temp;
    return 0;
}

CommonEvent compile_wl_to_cev(VEC_WLInst *arr, Arena *arena) {

    CommonEvent cev;
    cev_init(&cev, arena);

    for (size_t i = 0; i < arr->count; i++) {
        VEC_int32_t i_vec;
        VEC_StringView s_vec;
    
        WLOperand *operands = arr->at[i].operands.at;
        switch (arr->at[i].op) {
            case WL_VAR: {
                assert(operands[0].type == OPERAND_LOCAL ||
                       operands[0].type == OPERAND_TMP);
            
                // set_last_i_vself(operands[0].as.vself);
            
                int32_t dest = operands[0].as.vself + I_BASE;

                VEC_INIT(i_vec);

                int32_t a = disable_rc(arena, &cev, operands[1]);
                int32_t b = disable_rc(arena, &cev, operands[2]);

                VEC_PUSH(i_vec, operands[0].as.vself, arena);
                VEC_PUSH(i_vec, a, arena);
                VEC_PUSH(i_vec, b, arena);
                VEC_PUSH(i_vec, operands[3].as.integer, arena);
                
                VEC_INIT(s_vec);

                cev_push_cmd(&cev, CMD_VAR, indent, i_vec, s_vec);
                break;
            }
            // TODO
            case WL_INT_IF_HEAD:
                cev_push_simple_cmd(&cev, indent, CMD_IF_INT);
                break;

            case WL_LOOP:
                cev_push_simple_cmd(&cev, indent, CMD_LOOP);
                break;
            case WL_LOOP_N: {
                VEC_INIT(i_vec);
                int32_t n = disable_rc(arena, &cev, operands[0]);
                VEC_PUSH(i_vec, n, arena);
                
                VEC_INIT(s_vec);

                cev_push_cmd(&cev, indent, CMD_LOOP, i_vec, s_vec);
                break;
            }
            case WL_LOOP_END:
                cev_push_simple_cmd(&cev, indent, CMD_LOOP_END);
                break;
            default:
                UNREACHABLE;
        }
    }
    return cev;
}
