#ifndef WOD_ENVIRONMENT_H_
#define WOD_ENVIRONMENT_H_

#include "common.h"
#include "location.h"

typedef enum {
    TYPE_NONE,
    TYPE_ERROR,

    // Uses is_constexpr.
    TYPE_VOID,
    TYPE_INT,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_PTR,
    TYPE_FUNC,
    TYPE_DBDATA,
    TYPE_ARRAY,

    // Doesn't use is_constexpr.
    TYPE_DBTYPE,
    TYPE_CEVTYPE,
    TYPE_MODULE,
} BaseType;

typedef struct WodType WodType;
VEC_DEF(WodType);

typedef struct Symbol Symbol;
VEC_DEF(Symbol);
VEC_PTR_DEF(Symbol);

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

    // If constant value.
    bool is_constexpr;

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
            bool is_exaddr;
        };

        // If TYPE_CEVTYPE:
        StringView typename;

        // If TYPE_DBTYPE:
        struct {
            DBKind db_kind;
            Environment *db_fields;
            Environment *db_named_data;
        };

        // If TYPE_DBDATA:
        Symbol *db_type;

        // If TYPE_MODULE:
        Environment *module_env;
    };
};

struct Symbol {
    Environment *env;

    StringView name;
    WodType type;

    // Whether or not the symbol was declared with an initializer or
    // otherwise defined at some point.
    // Some checks rely on this information, for example to make sure
    // that a DB data element is properly defined at some point
    // in the program, even if that definition is deferred after
    // declaration.
    bool defined;

    // For error reporting.
    Location declaration;

    // Path of the file the symbol is in. Only valid if symbol is
    // top-level; otherwise is `SV_NULL`.
    // Used for resolving cross-file references.
    StringView top_level_path;

    union {
        // Keeps track of offsets for locals and DB fields.
        //
        // For locals, these are populated in the AST2WIR phase
        // because the tree traversal needs to simulate a compile-time stack
        // and emit push/pop instructions for the WIR phase.
        //
        // For DB fields, these are populated in the typechecking phase, because
        // DBs only have one scope and there is no need to worry about pushing
        // or popping.
        size_t local_offset;

        // Used to store compile-time values.
        int32_t const_i;
        StringView const_s;
        bool const_b;
    };
};

void env_init(Environment *env);
Environment *env_new(Environment *parent, Arena *arena);
Symbol *env_insert(Environment *env, StringView name, WodType type, Location declaration, bool defined, Arena *arena);
Symbol *env_find(Environment *env, StringView name);
Symbol *env_find_recursive(Environment *env, StringView name);


#endif // WOD_ENVIRONMENT_H_