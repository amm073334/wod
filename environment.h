#ifndef WOD_ENVIRONMENT_H_
#define WOD_ENVIRONMENT_H_

#include "common.h"
#include "location.h"

typedef enum {
    TYPE_NONE,
    TYPE_ERROR,

    // Uses is_compile_time.
    TYPE_VOID,
    TYPE_INT,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_PTR,
    TYPE_FUNC,
    TYPE_DBDATA,
    TYPE_ARRAY,

    // Doesn't use is_compile_time.
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
VEC_PTR_DEF(Environment);

struct WodType {
    BaseType basetype;

    // If can belong on the left side of an assignment.
    bool is_assignable;

    // If constant value, inline function, etc.
    bool is_compile_time;

    union {
        // If TYPE_ARRAY:
        struct {
            size_t array_len;
            WodType *array_of;
        };

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
    Environment *env;

    StringView name;
    WodType type;
    
    // For use in printing redeclaration errors.
    Location declared_at;

    // Path of the file the symbol is in. Only valid if symbol is
    // top-level; otherwise is `SV_NULL`.
    // Used for resolving cross-file references.
    StringView top_level_path;

    union {
        // Populated in the AST2WIR phase.
        // Acts as a function-local identifier.
        size_t offset;

        // Used to store compile-time values.
        int32_t const_i;
        StringView const_s;
        bool const_b;
    };
};

void env_init(Environment *env);
Environment *env_new(Environment *parent, Arena *arena);
Symbol *env_insert(Environment *env, StringView name, WodType type, Arena *arena);
Symbol *env_find(Environment *env, StringView name);
Symbol *env_find_recursive(Environment *env, StringView name);


#endif // WOD_ENVIRONMENT_H_