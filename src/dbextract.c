// Simple tool to convert text DB output into a function that returns the compiler's
// internal format for it.
// Doesn't really do any error checking.
// Basically, the purpose of this is to quickly extract the contents of the SDB,
// since it has a large number of hardcoded values.
// There are some minor known bugs:
// - If a quotation mark is escaped in a data item (as ""), this is not correctly converted.
// - If a quotation mark is present at all it will probably break things.
// These issues shouldn't affect converting the base SDB, but would cause issues elsewhere.

#include "common.h"
#include "source.h"
#include "db.h"

typedef struct {
    StringView name;
    StringView value;
} Pair;

void skip_line(const char **c) {
    while (**c && **c != '\n') (*c)++;
    if (**c == '\n') (*c)++;
}

Pair get_pair(Arena *arena, const char **c) {
    const char *start = *c;
    while (**c && **c != '=') (*c)++;
    StringView name = {
        .data = start,
        .len = *c - start
    };
    (*c)++;

    start = *c;
    size_t needed_escape_chars = 0;
    while (**c && **c != '\r' && **c != '\n') {
        if (**c == '\\') needed_escape_chars++;
        (*c)++;
    }
    StringView value = {
        .data = start,
        .len = *c - start
    };
    skip_line(c);

    char *escaped = arena_alloc_assert(arena, value.len + needed_escape_chars);

    for (size_t i = 0, j = 0; i < value.len; i++, j++) {
        escaped[j] = value.data[i];
        if (value.data[i] == '\\') {
            escaped[++j] = '\\';
        }
    }

    return (Pair){
        .name = name, .value = (StringView){
            .data = escaped,
            .len = value.len + needed_escape_chars
        }
    };
}

int32_t get_num(Arena *arena, const char **c) {
    Pair p = get_pair(arena, c);
    int32_t out;
    bool success = sv_to_int(arena, p.value, &out);
    assert(success);
    return out;
}

StringView get_str(Arena *arena, const char **c) {
    return get_pair(arena, c).value;
}

void convert_db_type(Arena *arena, const char **c, FILE *stream) {
    int32_t TYPE_ID = get_num(arena, c);
    int32_t DATANAME_LOAD_TYPE = get_num(arena, c);
    StringView DATANAME_LOAD_NAME = get_str(arena, c);
    int32_t ITEM_NUM = get_num(arena, c);

    VEC_DBItemDef itemdef = VEC_EMPTY;
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        VEC_PUSH(itemdef, ((DBItemDef){
            .CHOICE_NAME = VEC_EMPTY, .CHOICE_VAL = VEC_EMPTY}), arena);
        int32_t ty = get_num(arena, c);
        if (ty >= 2000) itemdef.at[i].type = DBITEM_STR;
        else itemdef.at[i].type = DBITEM_INT;
    }

    skip_line(c);
    skip_line(c);

    int32_t DATA_NUM = get_num(arena, c);
    StringView TYPENAME = get_str(arena, c);

    assert(get_num(arena, c) == ITEM_NUM);
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        itemdef.at[i].ITEMNAME = get_str(arena, c);
    }

    StringView MEMO = get_str(arena, c);

    assert(get_num(arena, c) == 100);
    for (int32_t i = 0; i < 100; i++) {
        int32_t num = get_num(arena, c);
        if (i < ITEM_NUM)
            itemdef.at[i].LOADTYPE = num;
    }

    assert(get_num(arena, c) == ITEM_NUM);
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        itemdef.at[i].VMEMO = get_str(arena, c);
    }
    
    assert(get_num(arena, c) == ITEM_NUM);
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        int32_t name_num = get_num(arena, c);
        for (int32_t j = 0; j < name_num; j++) {
            VEC_PUSH(itemdef.at[i].CHOICE_NAME, get_str(arena, c), arena);
        }
    }

    assert(get_num(arena, c) == ITEM_NUM);
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        int32_t val_num = get_num(arena, c);
        for (int32_t j = 0; j < val_num; j++) {
            VEC_PUSH(itemdef.at[i].CHOICE_VAL, get_num(arena, c), arena);
        }
    }
    
    assert(get_num(arena, c) == ITEM_NUM);
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        itemdef.at[i].DEFAULT_VAL = get_num(arena, c);
    }

    skip_line(c);
    skip_line(c);

    VEC_DBData data_vec = VEC_EMPTY;
    for (int32_t i = 0; i < DATA_NUM; i++) {
        DBData data = {.values = VEC_EMPTY};
        for (int32_t j = 0; j < ITEM_NUM; j++) {
            if (itemdef.at[j].type == DBITEM_STR) {
                assert(**c == '"');
                (*c)++;
                const char *start = *c;
                while (**c) {
                    if ((*c)[0] == '"') {
                        if ((*c)[1] == '"') {
                            // Escaped quotation mark.
                            (*c)++;
                        } else if ((*c)[1] == ',') {
                            // End of item.
                            break;
                        }
                    } 
                    (*c)++;
                }
                VEC_PUSH(data.values, ((DBVal){
                    .str_val = {.data = start, .len = *c - start}}), arena);
    
                (*c)++;
            } else {
                const char *start = *c;
                while (**c && **c != ',') (*c)++;
                StringView s = {.data = start, .len = *c - start};
                int32_t out;
                bool success = sv_to_int(arena, s, &out);
                assert(success);
    
                VEC_PUSH(data.values, ((DBVal){
                    .int_val = out}), arena);
                
            }
            (*c)++;
        }

        (*c) += sizeof("<<!--DATANAME--!>>") - 1;

        const char *start = *c;
        while (**c && **c != ',') (*c)++;
        data.name = (StringView){.data = start, .len = *c - start};

        VEC_PUSH(data_vec, data, arena);
        skip_line(c);
    }
    
    skip_line(c);
    skip_line(c);
    skip_line(c);

    fprintf(stream,
        "    {\n"
        "        DBType ty = {\n"
        "            .TYPE_ID = %d,\n"
        "            .TYPENAME = SV(\"" SV_FMT "\"),\n"
        "            .DATANAME_LOAD_TYPE = %d,\n"
        "            .DATANAME_LOAD_NAME = SV(\"" SV_FMT "\"),\n"
        "            .MEMO = SV(\"" SV_FMT "\"),\n"
        "            .itemdef = VEC_EMPTY,\n"
        "            .data = VEC_EMPTY,\n"
        "        };\n",
        TYPE_ID, 
        SV_FMT_VAL(TYPENAME), 
        DATANAME_LOAD_TYPE, 
        SV_FMT_VAL(DATANAME_LOAD_NAME),
        SV_FMT_VAL(MEMO)    
    );
    
    for (int32_t i = 0; i < ITEM_NUM; i++) {
        fprintf(stream, 
            "        {\n"
            "            DBItemDef def = {\n"
            "                .type = %d,\n"
            "                .ITEMNAME = SV(\"" SV_FMT "\"),\n"
            "                .VMEMO = SV(\"" SV_FMT "\"),\n"
            "                .LOADTYPE = %d,\n"
            "                .CHOICE_NAME = VEC_EMPTY,\n"
            "                .CHOICE_VAL = VEC_EMPTY,\n"
            "                .DEFAULT_VAL = %d,\n"
            "            };\n",
            itemdef.at[i].type,
            SV_FMT_VAL(itemdef.at[i].ITEMNAME), 
            SV_FMT_VAL(itemdef.at[i].VMEMO), 
            itemdef.at[i].LOADTYPE,
            itemdef.at[i].DEFAULT_VAL
        );

        for (size_t j = 0; j < itemdef.at[i].CHOICE_NAME.count; j++) {
            fprintf(stream, "            VEC_PUSH(def.CHOICE_NAME, SV(\"" SV_FMT "\"), arena);\n", SV_FMT_VAL(itemdef.at[i].CHOICE_NAME.at[j]));
        }

        for (size_t j = 0; j < itemdef.at[i].CHOICE_VAL.count; j++) {
            fprintf(stream, "            VEC_PUSH(def.CHOICE_VAL, %d, arena);\n", itemdef.at[i].CHOICE_VAL.at[j]);
        }
        fprintf(stream, "            VEC_PUSH(ty.itemdef, def, arena);\n");
        fprintf(stream, "        }\n");
    }

    for (int32_t i = 0; i < DATA_NUM; i++) {
        fprintf(stream, 
            "        {\n"
            "            DBData data = {\n"
            "                .name = SV(\"" SV_FMT "\"),\n"
            "                .values = VEC_EMPTY,\n"
            "            };\n", SV_FMT_VAL(data_vec.at[i].name));

        for (size_t j = 0; j < data_vec.at[i].values.count; j++) {
            if (itemdef.at[j].type == DBITEM_INT)
                fprintf(stream, "            VEC_PUSH(data.values, (DBVal){.int_val = %d}, arena);\n",
                    data_vec.at[i].values.at[j].int_val);
            else
                fprintf(stream, "            VEC_PUSH(data.values, (DBVal){.str_val = SV(\"" SV_FMT "\")}, arena);\n",
                    SV_FMT_VAL(data_vec.at[i].values.at[j].str_val));
        }
        fprintf(stream, "            VEC_PUSH(ty.data, data, arena);\n");
        fprintf(stream, "        }\n");
    }

    fprintf(stream, "        VEC_PUSH(db, ty, arena);\n");
    fprintf(stream, "    }\n");
}

int main(int argc, const char *argv[]) {
    if (argc != 2)
        return 1;

    Arena arena;

    Source *source = alloc_source(to_sv(argv[1]), &arena);
    if (!source)
        return 1;

    FILE *out = fopen("dbextract_out", "wb");
    if (!out)
        return 1;

    fprintf(out,
        "VEC_DBType sdb_xxxx(Arena *arena) {\n"
        "    VEC_DBType db = VEC_EMPTY;\n");

    const char *c = source->text.data;
    skip_line(&c);
    int32_t TYPE_NUM = get_num(&arena, &c);
    skip_line(&c);

    for (int32_t i = 0; i < TYPE_NUM; i++) {
        convert_db_type(&arena, &c, out);
    }

    fprintf(out,
        "}\n");

    return 0;
}
