#include "db.h"

void db_init(DB *db) {
    db->TYPE_ID = 0;
    db->DATANAME_LOAD_TYPE = 0;
    db->DATANAME_LOAD_NAME = SV("");
    db->TYPENAME = SV("");
    db->MEMO = SV("");
    memset(db->ITEMTYPE, 0, sizeof(db->ITEMTYPE));
    VEC_INIT(db->fields);
}

void db_write_txt(DB *db, FILE *stream) {
    fprintf(stream, "TYPE_ID=%d\n", db->TYPE_ID);
    fprintf(stream, "DATANAME_LOAD_TYPE=%d\n", db->DATANAME_LOAD_TYPE);
    fprintf(stream, "DATANAME_LOAD_TYPE=%d\n", db->DATANAME_LOAD_TYPE);
    fprintf(stream, "ITEM_NUM=%zu\n", db->fields.count);

    size_t n_int_prop = 0;
    size_t n_str_prop = 0;
    for (size_t i = 0; i < db->fields.count; i++) {
        if (db->fields.at[i].type == DBFIELD_INT) {
            fprintf(stream, "DATATYPE_%zu=%zu\n", i,
                db->fields.at[i].type + (n_int_prop++));
        } else {
            fprintf(stream, "DATATYPE_%zu=%zu\n", i,
                db->fields.at[i].type + (n_str_prop++));
        } 
    }

    fprintf(stream, "NUMDATA_TYPE_NUM=%zu\n", n_int_prop);
    fprintf(stream, "STRDATA_TYPE_NUM=%zu\n", n_str_prop);

    fprintf(stream, "DATA_NUM=1\n");
    
    fprintf(stream, "TYPENAME=" SV_FMT "\n", SV_FMT_VAL(db->TYPENAME));
    fprintf(stream, "ITEMNAME_NUM=%zu\n", db->fields.count);
    
    for (size_t i = 0; i < db->fields.count; i++)
        fprintf(stream, "ITEMNAME%zu=" SV_FMT "\n", i,
            SV_FMT_VAL(db->fields.at[i].ITEMNAME));
    
    fprintf(stream, "MEMO=" SV_FMT "\n", SV_FMT_VAL(db->MEMO));
    
    fprintf(stream, "ITEMTYPE_NUM=%zu\n", ARRLEN(db->ITEMTYPE));
    for (size_t i = 0; i < ARRLEN(db->ITEMTYPE); i++)
        fprintf(stream, "ITEM_LOADTYPE_%zu=%d\n", i, 0);
    
    fprintf(stream, "VMEMO_NUM=%zu\n", db->fields.count);
    for (size_t i = 0; i < db->fields.count; i++)
        fprintf(stream, "VMEMO_NAME_%zu=" SV_FMT "\n", i,
            SV_FMT_VAL(db->fields.at[i].VMEMO));
    
    fprintf(stream, "CHOICE_NUM=%zu\n", db->fields.count);
    for (size_t i = 0; i < db->fields.count; i++)
        fprintf(stream, "CHOICE_NAME_%zu_NUM=%d\n", i, 0);

    fprintf(stream, "CHOICE_VAL_NUM=%zu\n", db->fields.count);
    for (size_t i = 0; i < db->fields.count; i++)
        fprintf(stream, "CHOICE_VAL_%zu_NUM=%d\n", i, 0);

    fprintf(stream, "DEFAULT_VAL_NUM=%zu\n", db->fields.count);
    for (size_t i = 0; i < db->fields.count; i++)
        fprintf(stream, "DEFAULT_VAL_%zu=%d\n", i,
            db->fields.at[i].DEFAULT_VAL);

    fprintf(stream, "<<--CSV_START-->>\n");

    for (size_t i = 0; i < db->fields.count; i++) {
        fprintf(stream, "\"" SV_FMT "\",",
            SV_FMT_VAL(db->fields.at[i].ITEMNAME));
    }
    fprintf(stream, "\n");

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
    for (size_t i = 0; i < db->fields.count; i++) {
        if (db->fields.at[i].type == DBFIELD_INT)
            fprintf(stream, "%d,", db->fields.at[i].DEFAULT_VAL);
        else
            fprintf(stream, "\"\",");
    }
    fprintf(stream, "<<!--DATANAME--!>>,\n");

    fprintf(stream, "\n");
    fprintf(stream, "<<--CSV_END-->>\n");
}