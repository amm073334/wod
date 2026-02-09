#ifndef WOD_ENVIRONMENT_H_
#define WOD_ENVIRONMENT_H_

#include "common.h"

typedef enum {
    TYPE_VOID,
    TYPE_INT,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_FUNC,
    TYPE_CDB,
    TYPE_MODULE,

    TYPE_SM_ANY,
    TYPE_SM_ANY_CALL,
    TYPE_SM_ANY_ARGS,
    TYPE_SM_STATE,
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

// Not very memory-efficient, but oh well.
struct WodType {
    BaseType basetype;

    union {
        size_t array_size;

        // If TYPE_FUNC:
        struct {
            BaseType return_type;
            VEC_WodType params;
        };

        // If TYPE_CDB or TYPE_MODULE:
        Environment *env;
    };
};

struct Symbol {
    WodType type;
    StringView name;
    void *value;
};

void env_init(Environment *env);
Environment *env_new(Environment *parent, Arena *arena);
Symbol *env_insert(Environment *env, StringView name, BaseType basetype, void *value, Arena *arena);
Symbol *env_find(Environment *env, StringView name);

#endif // WOD_ENVIRONMENT_H_