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
        Environment env;
    };
};

struct Symbol {
    WodType type;
    StringView name;
    int32_t offset;
};

void env_init(Environment *env);
Environment *env_new(Environment *parent, Arena *arena);
bool env_insert(Environment *env, StringView name, int32_t offset, Arena *arena);
int32_t *env_get(Environment *env, StringView name);

#endif // WOD_ENVIRONMENT_H_