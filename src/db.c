#include "db.h"

void db_init(DBType *db) {
    db->TYPE_ID = 0;
    db->DATANAME_LOAD_TYPE = 0;
    db->DATANAME_LOAD_NAME = SV("");
    db->TYPENAME = SV("");
    db->MEMO = SV("");
    VEC_INIT(db->itemdef);
    VEC_INIT(db->data);
}

VEC_DBType db_sys_3713(Arena *arena) {
    VEC_DBType db = VEC_EMPTY;

    // 0: マップ設定
    DBType sys_0;
    db_init(&sys_0);
    sys_0.TYPENAME = SV("マップ設定");

    VEC_PUSH(db, sys_0, arena);
}

bool db_write_txt(DBType *db, FILE *stream) {
    fprintf(stream, "TYPE_ID=%d\n", db->TYPE_ID);
    fprintf(stream, "DATANAME_LOAD_TYPE=%d\n", db->DATANAME_LOAD_TYPE);
    fprintf(stream, "DATANAME_LOAD_NAME=%d\n", db->DATANAME_LOAD_NAME);
    fprintf(stream, "ITEM_NUM=%zu\n", db->itemdef.count);

    size_t n_int_items = 0;
    size_t n_str_items = 0;
    for (size_t i = 0; i < db->itemdef.count; i++) {
        if (db->itemdef.at[i].type == DBITEM_INT) {
            fprintf(stream, "DATATYPE_%zu=%zu\n", i,
                db->itemdef.at[i].type + (n_int_items++));
        } else {
            fprintf(stream, "DATATYPE_%zu=%zu\n", i,
                db->itemdef.at[i].type + (n_str_items++));
        } 
    }

    fprintf(stream, "NUMDATA_TYPE_NUM=%zu\n", n_int_items);
    fprintf(stream, "STRDATA_TYPE_NUM=%zu\n", n_str_items);

    fprintf(stream, "DATA_NUM=%zu\n", db->data.count);
    
    fprintf(stream, "TYPENAME=" SV_FMT "\n", SV_FMT_VAL(db->TYPENAME));
    fprintf(stream, "ITEMNAME_NUM=%zu\n", db->itemdef.count);
    
    for (size_t i = 0; i < db->itemdef.count; i++)
        fprintf(stream, "ITEMNAME%zu=" SV_FMT "\n", i,
            SV_FMT_VAL(db->itemdef.at[i].ITEMNAME));
    
    fprintf(stream, "MEMO=" SV_FMT "\n", SV_FMT_VAL(db->MEMO));
    
    // There are always 100 of these regardless of how many items are
    // actually specified. Items that are not specified are simply given
    // loadtype 0.
    fprintf(stream, "ITEMTYPE_NUM=100\n");
    for (size_t i = 0; i < 100; i++) {
        int32_t loadtype = i < db->itemdef.count ?
            db->itemdef.at[i].loadtype : 0;

        fprintf(stream, "ITEM_LOADTYPE_%zu=%d\n", i, loadtype);
    }
    
    fprintf(stream, "VMEMO_NUM=%zu\n", db->itemdef.count);
    for (size_t i = 0; i < db->itemdef.count; i++)
        fprintf(stream, "VMEMO_NAME_%zu=" SV_FMT "\n", i,
            SV_FMT_VAL(db->itemdef.at[i].VMEMO));
    
    fprintf(stream, "CHOICE_NUM=%zu\n", db->itemdef.count);
    for (size_t i = 0; i < db->itemdef.count; i++) {
        DBItemDef *item = &db->itemdef.at[i];
        fprintf(stream, "CHOICE_NAME_%zu_NUM=%d\n", i, item->CHOICE_NAME.count);
        for (size_t j = 0; j < item->CHOICE_NAME.count; j++) {
            fprintf(stream, "CHOICE_NAME_%zu_%zu=" SV_FMT "\n",
                i, j, SV_FMT_VAL(item->CHOICE_NAME.at[j]));
        }
    }

    fprintf(stream, "CHOICE_VAL_NUM=%zu\n", db->itemdef.count);
    for (size_t i = 0; i < db->itemdef.count; i++) {
        DBItemDef *item = &db->itemdef.at[i];
        fprintf(stream, "CHOICE_VAL_%zu_NUM=%d\n", i, item->CHOICE_VAL.count);
        for (size_t j = 0; j < item->CHOICE_VAL.count; j++) {
            fprintf(stream, "CHOICE_VAL_%zu_%zu=%d\n",
                i, j, item->CHOICE_VAL.at[j]);
        }
    }

    fprintf(stream, "DEFAULT_VAL_NUM=%zu\n", db->itemdef.count);
    for (size_t i = 0; i < db->itemdef.count; i++)
        fprintf(stream, "DEFAULT_VAL_%zu=%d\n", i,
            db->itemdef.at[i].DEFAULT_VAL);

    fprintf(stream, "<<--CSV_START-->>\n");

    for (size_t i = 0; i < db->itemdef.count; i++) {
        fprintf(stream, "\"" SV_FMT "\",",
            SV_FMT_VAL(db->itemdef.at[i].ITEMNAME));
    }
    fprintf(stream, "\n");

    if (db->data.count == 0) {
        // When loading a DB from file, it always gets at least one element
        // even if the loaded text file states there are 0 elements.
        //
        // In particular, if the text file lists no elements, the
        // element that is automatically generated will have 0 in all
        // its integer fields, which is slightly odd if default values
        // have been set.
        //
        // So if there will be one element anyway, just emit that element
        // here and with the expected default values.
        for (size_t i = 0; i < db->itemdef.count; i++) {
            if (db->itemdef.at[i].type == DBITEM_INT)
                fprintf(stream, "%d,", db->itemdef.at[i].DEFAULT_VAL);
            else
                fprintf(stream, "\"\",");
        }
        fprintf(stream, "<<!--DATANAME--!>>,\n");
    } else {
        for (size_t i = 0; i < db->data.count; i++) {
            DBData *data = &db->data.at[i];
            
            if (data->values.count != db->itemdef.count);
                return false;
            
            for (size_t j = 0; j < data->values.count; j++) {
                if (db->itemdef.at[i].type == DBITEM_INT)
                    fprintf(stream, "%d,", data->values.at[j].int_val);
                else
                    fprintf(stream, "\"" SV_FMT "\",", SV_FMT_VAL(data->values.at[j].str_val));
            }
    
            fprintf(stream, "<<!--DATANAME--!>>" SV_FMT ",\n", SV_FMT_VAL(data->name));
        }
    }

    fprintf(stream, "\n");
    fprintf(stream, "<<--CSV_END-->>\n");
}