#ifndef WOD_CEV_H_
#define WOD_CEV_H_

#include <stdio.h>

#include "common.h"
#include "command.h"

typedef enum {
    COLOR_BLACK,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_PURPLE,
    COLOR_YELLOW,
    COLOR_GRAY
} CevColor;

typedef enum {
    TR_CALL_ONLY,
    TR_ON_CONDITION,
    TR_PARALLEL_ON_CONDITION,
    TR_PARALLEL_ALWAYS
} CevTriggerType;

typedef enum {
    TC_GT   = 0x00,
    TC_GTE  = 0x10,
    TC_EQ   = 0x20,
    TC_LTE  = 0x30,
    TC_LT   = 0x40,
    TC_NEQ  = 0x50,
    TC_AND  = 0x60
} CevTriggerComp;

typedef enum {
    LT_NONE,
    LT_DB,
    LT_ENUM
} CevLoadType;

VEC_DEF(Command);

typedef struct {
    // Common event data: names are exactly as in the official text output,
    // with numbers swapped out for lowercase `i` or `j`.

#define VALINPUT_NAME_NUM 11
#define LOADTYPE_NUM 10
#define CHOICE_STR_NUM 10
#define CHOICE_VAL_NUM 10
#define DEFAULT_VAL_NUM 5

    int32_t COMMON_ID;
    StringView COMMON_NAME;
    CevColor COMMAND_COLOR;

    int8_t TRIGGER;
    int32_t TRIGGER_TARGET;
    int32_t TRIGGER_VAL;

    StringView MEMO;

    int8_t VALINPUT_NUM;
    int8_t STRINPUT_NUM;

    StringView VALINPUT_NAME_i[VALINPUT_NAME_NUM];
    
    CevLoadType LOADTYPEi[LOADTYPE_NUM];
    
    int32_t CHOICE_STR_i_NUM[CHOICE_STR_NUM];
    VEC_StringView CHOICE_STR_i_j;
    int32_t CHOICE_VAL_i_NUM[CHOICE_VAL_NUM];
    VEC_int32_t CHOICE_VAL_i_j;
    
    int32_t DEFAULT_VAL_i[DEFAULT_VAL_NUM];

    StringView RETURN_VAL_NAME;
    int32_t RETURN_VAL_TARGET;

    StringView CSELF_NAME_i[100];

    // Auxiliary data not named in real editor output.
    VEC_Command commands;
    
    // Arena to hold all the vectors.
    Arena *arena;
} CommonEvent;

void cev_init(CommonEvent *cev, Arena *arena);

Command *cev_push_simple_cmd(CommonEvent *cev,
    uint8_t indent, int32_t command_id);

Command *cev_push_cmd(CommonEvent *cev,
    int32_t command_id, uint8_t indent,
    VEC_int32_t int_fields, VEC_StringView str_fields);

void cev_write_txt(CommonEvent *cev, FILE *stream);

#endif // WOD_CEV_H_