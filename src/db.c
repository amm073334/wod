#include "db.h"

void db_init(DBType *ty) {
    ty->TYPE_ID = 0;
    ty->DATANAME_LOAD_TYPE = 0;
    ty->DATANAME_LOAD_NAME = SV("");
    ty->TYPENAME = SV("");
    ty->MEMO = SV("");
    VEC_INIT(ty->itemdef);
    VEC_INIT(ty->data);
}

bool db_write_txt(DBType *ty, FILE *stream) {
    fprintf(stream, "TYPE_ID=%d\n", ty->TYPE_ID);
    fprintf(stream, "DATANAME_LOAD_TYPE=%d\n", ty->DATANAME_LOAD_TYPE);
    fprintf(stream, "DATANAME_LOAD_NAME=" SV_FMT "\n", SV_FMT_VAL(ty->DATANAME_LOAD_NAME));
    fprintf(stream, "ITEM_NUM=%zu\n", ty->itemdef.count);

    size_t n_int_items = 0;
    size_t n_str_items = 0;
    for (size_t i = 0; i < ty->itemdef.count; i++) {
        if (ty->itemdef.at[i].type == DBITEM_INT) {
            fprintf(stream, "DATATYPE_%zu=%zu\n", i,
                ty->itemdef.at[i].type + (n_int_items++));
        } else {
            fprintf(stream, "DATATYPE_%zu=%zu\n", i,
                ty->itemdef.at[i].type + (n_str_items++));
        } 
    }

    fprintf(stream, "NUMDATA_TYPE_NUM=%zu\n", n_int_items);
    fprintf(stream, "STRDATA_TYPE_NUM=%zu\n", n_str_items);

    fprintf(stream, "DATA_NUM=%zu\n", ty->data.count);
    
    fprintf(stream, "TYPENAME=" SV_FMT "\n", SV_FMT_VAL(ty->TYPENAME));
    fprintf(stream, "ITEMNAME_NUM=%zu\n", ty->itemdef.count);
    
    for (size_t i = 0; i < ty->itemdef.count; i++)
        fprintf(stream, "ITEMNAME%zu=" SV_FMT "\n", i,
            SV_FMT_VAL(ty->itemdef.at[i].ITEMNAME));
    
    fprintf(stream, "MEMO=" SV_FMT "\n", SV_FMT_VAL(ty->MEMO));
    
    // There are always 100 of these regardless of how many items are
    // actually specified. Items that are not specified are simply given
    // loadtype 0.
    fprintf(stream, "ITEMTYPE_NUM=100\n");
    for (size_t i = 0; i < 100; i++) {
        int32_t loadtype = i < ty->itemdef.count ?
            ty->itemdef.at[i].LOADTYPE : 0;

        fprintf(stream, "ITEM_LOADTYPE_%zu=%d\n", i, loadtype);
    }
    
    fprintf(stream, "VMEMO_NUM=%zu\n", ty->itemdef.count);
    for (size_t i = 0; i < ty->itemdef.count; i++)
        fprintf(stream, "VMEMO_NAME_%zu=" SV_FMT "\n", i,
            SV_FMT_VAL(ty->itemdef.at[i].VMEMO));
    
    fprintf(stream, "CHOICE_NUM=%zu\n", ty->itemdef.count);
    for (size_t i = 0; i < ty->itemdef.count; i++) {
        DBItemDef *item = &ty->itemdef.at[i];
        fprintf(stream, "CHOICE_NAME_%zu_NUM=%zu\n", i, item->CHOICE_NAME.count);
        for (size_t j = 0; j < item->CHOICE_NAME.count; j++) {
            fprintf(stream, "CHOICE_NAME_%zu_%zu=" SV_FMT "\n",
                i, j, SV_FMT_VAL(item->CHOICE_NAME.at[j]));
        }
    }

    fprintf(stream, "CHOICE_VAL_NUM=%zu\n", ty->itemdef.count);
    for (size_t i = 0; i < ty->itemdef.count; i++) {
        DBItemDef *item = &ty->itemdef.at[i];
        fprintf(stream, "CHOICE_VAL_%zu_NUM=%zu\n", i, item->CHOICE_VAL.count);
        for (size_t j = 0; j < item->CHOICE_VAL.count; j++) {
            fprintf(stream, "CHOICE_VAL_%zu_%zu=%d\n",
                i, j, item->CHOICE_VAL.at[j]);
        }
    }

    fprintf(stream, "DEFAULT_VAL_NUM=%zu\n", ty->itemdef.count);
    for (size_t i = 0; i < ty->itemdef.count; i++)
        fprintf(stream, "DEFAULT_VAL_%zu=%d\n", i,
            ty->itemdef.at[i].DEFAULT_VAL);

    fprintf(stream, "<<--CSV_START-->>\n");

    for (size_t i = 0; i < ty->itemdef.count; i++) {
        fprintf(stream, "\"" SV_FMT "\",",
            SV_FMT_VAL(ty->itemdef.at[i].ITEMNAME));
    }
    fprintf(stream, "\n");

    if (ty->data.count == 0) {
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
        for (size_t i = 0; i < ty->itemdef.count; i++) {
            if (ty->itemdef.at[i].type == DBITEM_INT)
                fprintf(stream, "%d,", ty->itemdef.at[i].DEFAULT_VAL);
            else
                fprintf(stream, "\"\",");
        }
        fprintf(stream, "<<!--DATANAME--!>>,\n");
    } else {
        for (size_t i = 0; i < ty->data.count; i++) {
            DBData *data = &ty->data.at[i];
            
            if (data->values.count != ty->itemdef.count);
                return false;
            
            for (size_t j = 0; j < data->values.count; j++) {
                if (ty->itemdef.at[i].type == DBITEM_INT)
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