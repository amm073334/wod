#ifndef WOD_DB_H_
#define WOD_DB_H_

#include <stdio.h>

#include "common.h"

typedef enum {
    DBITEM_INT = 1000,
    DBITEM_STR = 2000,
} DBItemType;

typedef enum {
    DBITEM_LOADTYPE_DB_CHOICE0_SDB = 0,
    DBITEM_LOADTYPE_DB_CHOICE0_UDB = 1,
    DBITEM_LOADTYPE_DB_CHOICE0_CDB = 2,
    DBITEM_LOADTYPE_DB_CHOICE0_CEV = 3,
} DBItemLoadTypeDBChoice0;

// Specifies the 特殊設定 for a DB item, which configures which values can be selected for
// the item from the editor. These settings only change the editor interface and don't
// enforce any run-time constraints for legal values in a given DB item.
typedef enum {
    // "特殊な設定方法を使用しない"
    // Don't do anything special.
    DBITEM_LOADTYPE_DEFAULT = 0,

    // "ファイル読み込み（文字列）"
    // Shows a "File" button and allows you to browse a folder.
    //
    // CHOICE:
    // Exactly one name, whose value is equal to the folder name.
    // One boolean value (0 or 1) indicating whether or not
    // to tick the "保存時はフォルダ名を省く" box.
    DBITEM_LOADTYPE_FILE = 1,

    // "データベース参照"
    // Use another DB type's data elements or the common event list as an enum.
    //
    // CHOICE:
    // Either zero or three names, depending on the third value.
    // Three values:
    // 0: Indicates which DB type (or common event list) to use as the enum.
    //    Documented in `DBItemLoadTypeDBChoice0` above.
    // 1: Indicates the DB type's numeric ID.
    // 2: Boolean flag indicating whether or not to add elements -1 through -3.
    //    If this is set to 1, then there will be three names that name these elements.
    //    If it's set to 0, then there are no names.
    DBITEM_LOADTYPE_DB = 2,
    
    // Also lets you to use another DB type's data elements as an enum, 
    // but allows you to specify the DB type by name instead of numeric ID.
    // This option can't be used with the common event list as the enum.
    //
    // CHOICE:
    // Same as above. Notably, the second value is still set to the DB type's numeric
    // ID even though that information presumably does not matter here (since the entire
    // point of specifying the DB type by name is to avoid any problems if the numeric
    // ID updates).
    DBITEM_LOADTYPE_DB_NAME = 4,

    // "選択肢を手動作成（数値）"
    // Make a custom enum.
    //
    // CHOICE:
    // One (name, value) pair for each enum entry, corresponding to its name and its
    // actual integer value.
    DBITEM_LOADTYPE_ENUM = 3,
} DBItemLoadType;

// DB-type-wide setting that sets how the type's data IDs are determined.
typedef enum {
    // Typed in manually.
    DB_LOADTYPE_MANUAL = 0,

    // ID of any given data element is set to the value of the first string-type item in the element.
    DB_LOADTYPE_FIRSTSTRING = 1,

    // Clones IDs from the immediately preceding DB type.
    DB_LOADTYPE_CLONE_PREVIOUS = 2,

    // Clones IDs from the given DB type, specified by numeric type ID.
    // The type ID is specified by _adding_ that ID value to one of the following flags.
    DB_LOADTYPE_CLONE_NUMBER_SDB = 10000,
    DB_LOADTYPE_CLONE_NUMBER_UDB = 20000,
    DB_LOADTYPE_CLONE_NUMBER_CDB = 30000,

    // Clones IDs from the given DB type, specified by type name.
    // In this case, the loadtype value is exactly one of the following, without an
    // added ID value. The type name is specified by filling in the DATANAME_LOAD_NAME string.
    DB_LOADTYPE_CLONE_NAME_SDB  = 110000,
    DB_LOADTYPE_CLONE_NAME_UDB  = 120000,
    DB_LOADTYPE_CLONE_NAME_CDB  = 130000,
} DataNameLoadTypeFlag;

// Basically a "field" or "property" of a DB type. Officially referred to as an "item."
typedef struct {
    // Integer or string. In the output, the number of integer or string items
    // up until that point is added to the value.
    DBItemType type;

    // Name.
    StringView ITEMNAME;
    
    // Seems to be unused.
    StringView VMEMO;

    // Specifies what values can be set in the editor.
    DBItemLoadType loadtype;

    // List of "choices." The exact semantics depends on the item's loadtype.
    VEC_StringView CHOICE_NAME;
    VEC_int32_t CHOICE_VAL;

    // If the item is of integer type, specifies its default value.
    int32_t DEFAULT_VAL;
} DBItemDef;
VEC_DEF(DBItemDef);

typedef union {
    StringView str_val;
    int32_t int_val;
} DBVal;
VEC_DEF(DBVal);

typedef struct {
    StringView name;
    VEC_DBVal values;
} DBData;
VEC_DEF(DBData);

// NOTE: Struct format is not necessarily accurate to official binary format;
//       integer widths need to be investigated.
typedef struct {
    // Index of the DB type in the corresponding DB.
    // This value doesn't seem to actually serve a purpose: Even if you keep all type IDs as 0,
    // the system will still function properly.
    int32_t TYPE_ID;

    // How IDs of data elements are specified.
    int32_t DATANAME_LOAD_TYPE;
    StringView DATANAME_LOAD_NAME;

    // Name.
    StringView TYPENAME;
    
    // Description.
    StringView MEMO;

    // Definition of DB items.
    VEC_DBItemDef itemdef;

    // Data elements. Each data element should have the same format as
    // defined in the item definition.
    VEC_DBData data;
} DBType;
VEC_DEF(DBType);

void db_init(DBType *db);
void db_init_sys_3713(VEC_DBType *db);

// Returns `false` if the DB was invalid. 
bool db_write_txt(DBType *db, FILE *stream);

#endif // WOD_DB_H_