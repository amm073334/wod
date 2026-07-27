#include "commonevent.h"

void cev_init(CommonEvent *cev, Arena *arena) {
    cev->COMMON_ID = 0;
    cev->COMMON_NAME = SV("");
    cev->COMMAND_COLOR = COLOR_BLACK;
    cev->TRIGGER = (uint8_t)TR_CALL_ONLY | (uint8_t)TC_EQ;
    cev->TRIGGER_TARGET = 2000000;
    cev->TRIGGER_VAL = 0;
    cev->MEMO = SV("");
    cev->VALINPUT_NUM = 0;
    cev->STRINPUT_NUM = 0;

    memset(&cev->VALINPUT_NAME_i, 0, sizeof(cev->VALINPUT_NAME_i));
    memset(&cev->LOADTYPEi, 0, sizeof(cev->LOADTYPEi));
    
    memset(&cev->CHOICE_STR_i_NUM, 0, sizeof(cev->CHOICE_STR_i_NUM));
    VEC_INIT(cev->CHOICE_STR_i_j);
    
    memset(&cev->CHOICE_VAL_i_NUM, 0, sizeof(cev->CHOICE_VAL_i_NUM));
    VEC_INIT(cev->CHOICE_VAL_i_j);

    memset(&cev->DEFAULT_VAL_i, 0, sizeof(cev->DEFAULT_VAL_i));

    cev->RETURN_VAL_NAME = SV("");
    cev->RETURN_VAL_TARGET = -1;
    
    for (size_t i = 0; i < ARRLEN(cev->CSELF_NAME_i); i++)
        cev->CSELF_NAME_i[i] = SV("");

    VEC_INIT(cev->commands);

    cev->arena = arena;
}

Command *cev_push_simple_cmd(CommonEvent *cev,
    int32_t command_id, uint8_t indent) {

    Command c;
    c.id = command_id;
    c.indent = indent;
    VEC_INIT(c.int_list);
    VEC_INIT(c.str_list);

    VEC_PUSH(cev->commands, c, cev->arena);
    return &cev->commands.at[cev->commands.count - 1];
}

Command *cev_push_cmd(CommonEvent *cev,
    int32_t command_id, uint8_t indent,
    VEC_int32_t int_fields, VEC_StringView str_fields) {

    Command c;
    c.id = command_id;
    c.indent = indent;
    c.int_list = int_fields;
    c.str_list = str_fields;

    VEC_PUSH(cev->commands, c, cev->arena);
    return &cev->commands.at[cev->commands.count - 1];
}

void cev_write_txt(CommonEvent *cev, FILE *stream) {

// Write identifier equal to itself.
#define WOD_CEV_WRITE_I(ID) do {                                            \
    fprintf(stream, #ID "=%d\n", cev->ID);                                  \
} while (0)
#define WOD_CEV_WRITE_S(ID) do {                                            \
    fprintf(stream, #ID "=" SV_FMT "\n", SV_FMT_VAL(cev->ID));              \
} while (0)

// Write macro-defined identifier.
#define WOD_CEV_WRITE_M(ID) do {                                            \
    fprintf(stream, #ID "=%d\n", ID);                                       \
} while (0)

// Write identifier equal to each value in corresponding i-array
#define WOD_CEV_WRITE_I_ARR(ID, I) do {                                     \
    for (size_t i = 0; i < I; i++) {                                        \
        fprintf(stream, #ID "%zu=%d\n", i, cev->ID##i[i]);                  \
    }                                                                       \
} while (0)
#define WOD_CEV_WRITE_S_ARR(ID, I) do {                                     \
    for (size_t i = 0; i < I; i++) {                                        \
        fprintf(stream, #ID "%zu=" SV_FMT "\n", i,                          \
            SV_FMT_VAL(cev->ID##i[i]));                                     \
    }                                                                       \
} while (0)

    WOD_CEV_WRITE_I(COMMON_ID);
    WOD_CEV_WRITE_S(COMMON_NAME);
    WOD_CEV_WRITE_I(COMMAND_COLOR);
    WOD_CEV_WRITE_I(TRIGGER);
    WOD_CEV_WRITE_I(TRIGGER_TARGET);
    WOD_CEV_WRITE_I(TRIGGER_VAL);
    WOD_CEV_WRITE_S(MEMO);
    WOD_CEV_WRITE_I(VALINPUT_NUM);
    WOD_CEV_WRITE_I(STRINPUT_NUM);
    WOD_CEV_WRITE_M(VALINPUT_NAME_NUM);
    WOD_CEV_WRITE_S_ARR(VALINPUT_NAME_, VALINPUT_NAME_NUM);
    WOD_CEV_WRITE_M(LOADTYPE_NUM);
    WOD_CEV_WRITE_I_ARR(LOADTYPE, LOADTYPE_NUM);

    WOD_CEV_WRITE_M(CHOICE_STR_NUM);
    for (int32_t i = 0; i < CHOICE_STR_NUM; i++) {
        fprintf(stream, "CHOICE_STR_%d_NUM=%d\n",
            i, cev->CHOICE_STR_i_NUM[i]);
        for (int32_t j = 0; j < cev->CHOICE_STR_i_NUM[i]; j++) {
            fprintf(stream, "CHOICE_STR_%d_%d=" SV_FMT "\n",
                i, j, SV_FMT_VAL(cev->CHOICE_STR_i_j.at[i]));
        }
    }

    WOD_CEV_WRITE_M(CHOICE_VAL_NUM);
    for (int32_t i = 0; i < CHOICE_VAL_NUM; i++) {
        fprintf(stream, "CHOICE_VAL_%d_NUM=%d\n",
            i, cev->CHOICE_VAL_i_NUM[i]);
        for (int32_t j = 0; j < cev->CHOICE_VAL_i_NUM[i]; j++) {
            fprintf(stream, "CHOICE_VAL_%d_%d=%d\n",
                i, j, cev->CHOICE_VAL_i_j.at[i]);
        }
    }
    
    WOD_CEV_WRITE_M(DEFAULT_VAL_NUM);
    WOD_CEV_WRITE_I_ARR(DEFAULT_VAL_, DEFAULT_VAL_NUM);
    WOD_CEV_WRITE_S(RETURN_VAL_NAME);
    WOD_CEV_WRITE_I(RETURN_VAL_TARGET);
    WOD_CEV_WRITE_S_ARR(CSELF_NAME_, ARRLEN(cev->CSELF_NAME_i));
    
#undef WOD_CEV_WRITE_I
#undef WOD_CEV_WRITE_S
#undef WOD_CEV_WRITE_M
#undef WOD_CEV_WRITE_I_ARR
#undef WOD_CEV_WRITE_S_ARR

    fprintf(stream, "COMMAND_NUM=%zu\n", cev->commands.count);
    fprintf(stream, "--\n");
    fprintf(stream, "WoditorEvCOMMAND_START\n");

    for (size_t i = 0; i < cev->commands.count; i++) {
        Command *c = &cev->commands.at[i];
        fprintf(stream, "[%d][%zu,%zu]<%d>(",
            c->id,
            c->int_list.count,
            c->str_list.count,
            c->indent);
        
        for (size_t j = 0; j < c->int_list.count; j++) {
            fprintf(stream, "%d", c->int_list.at[j]);
            if (j < c->int_list.count - 1)
                fprintf(stream, ",");
        }
        fprintf(stream, ")(");
        for (size_t j = 0; j < c->str_list.count; j++) {
            fprintf(stream, "\"" SV_FMT "\"", SV_FMT_VAL(c->str_list.at[j]));
            if (j < c->str_list.count - 1)
                fprintf(stream, ",");
        }
        fprintf(stream, ")\n");
    }
    fprintf(stream, "WoditorEvCOMMAND_END\n");
    fprintf(stream, "--\n");

    // Skip text generation, since it doesn't seem to have a functional purpose.
    fprintf(stream, "[COMMAND_TEXT_START]\n");
    fprintf(stream, "[COMMAND_TEXT_END]\n");
}