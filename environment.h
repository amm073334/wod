#ifndef WOD_ENVIRONMENT_H_
#define WOD_ENVIRONMENT_H_

#include "common.h"

typedef enum {
    TYPE_NONE,
    TYPE_ERROR,

    // Uses is_compile_time and array_length.
    TYPE_VOID,
    TYPE_INT,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_PTR,
    TYPE_FUNC,
    TYPE_DBDATA,

    // Doesn't use is_compile_time and array_length.
    TYPE_DBTYPE,
    TYPE_CEVTYPE,
    TYPE_MODULE,
} BaseType;

typedef struct WodType WodType;
VEC_DEF(WodType);

typedef struct Symbol Symbol;
VEC_DEF(Symbol);

typedef struct Environment Environment;
struct Environment {
    Environment *parent;
    VEC_Symbol symbols;
};

struct WodType {
    BaseType basetype;

    // If can belong on the left side of an assignment.
    bool is_assignable;

    // If constant value, inline function, etc.
    bool is_compile_time;

    // 0 means not an array type.
    size_t array_length;

    union {
        // If TYPE_PTR:
        WodType *ptr_to;

        // If TYPE_FUNC:
        struct {
            WodType *return_type;
            VEC_WodType params;
        };

        // If TYPE_CEVTYPE:
        StringView typename;

        // If TYPE_DBTYPE:
        struct {
            enum { DB_UDB, DB_CDB } db_kind;
            Environment *db_env;
        };

        // If TYPE_MODULE:
        Environment *module_env;

        // If TYPE_DBDATA:
        StringView db_name;
    };
};

struct Symbol {
    StringView name;
    WodType type;

    // This is for example the common event ID, or the CSelf ID.
    // Values are relative to the current file, not across all files.
    size_t offset;
};

void env_init(Environment *env);
Environment *env_new(Environment *parent, Arena *arena);
Symbol *env_insert(Environment *env, StringView name, WodType type, size_t offset, Arena *arena);
WodType *env_find(Environment *env, StringView name);

#endif // WOD_ENVIRONMENT_H_