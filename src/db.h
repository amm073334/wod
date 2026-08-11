#ifndef WOD_DB_H_
#define WOD_DB_H_

#include <stdio.h>

#include "common.h"

typedef enum {
    DBFIELD_INT = 1000,
    DBFIELD_STR = 2000,
} DBFieldType;

typedef struct {
    DBFieldType type;
    StringView ITEMNAME;
    StringView VMEMO;
    StringView CHOICE_NAME;
    int32_t CHOICE_VAL;
    int32_t DEFAULT_VAL;
} DBField;

VEC_DEF(DBField);

// NOTE: Struct format is not necessarily accurate to official binary format;
//       integer widths need to be investigated.
typedef struct {
    int32_t TYPE_ID;
    int32_t DATANAME_LOAD_TYPE;
    StringView DATANAME_LOAD_NAME;

    StringView TYPENAME;
    StringView MEMO;

    int32_t ITEMTYPE[100];

    VEC_DBField fields;
} DB;

void db_init(DB *db);
void db_write_txt(DB *db, FILE *stream);

#endif // WOD_DB_H_