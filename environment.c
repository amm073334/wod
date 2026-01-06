#include "environment.h"

void env_init(Environment *env) {
    env->parent = NULL;
    env->symbols.count = 0;
    env->symbols.capacity = 0;
    env->symbols.at = NULL;
}

Environment *env_new(Environment *parent, Arena *arena) {
    Environment *env = arena_alloc(arena, sizeof(Environment));
    if (!env)
        return NULL;

    env->symbols.count = 0;
    env->symbols.capacity = 0;
    env->symbols.at = NULL;

    env->parent = parent;

    return env;
}

// TODO: This would be more efficient with a hash table.
static Symbol *env_find(Environment *env, StringView name) {
    if (!env) return NULL;
    
    for (size_t i = 0; i < env->symbols.count; i++) {
        if (!sv_equals(name, env->symbols.at[i].name))
            continue;

        return &env->symbols.at[i];
    }

    return env_find(env->parent, name);
}

bool env_insert(Environment *env, StringView name, int32_t offset, Arena *arena) {
    Symbol *entry = env_find(env, name);
    
    if (!entry) {
        Symbol sym;
        sym.name = name;
        sym.offset = offset;
        sym.type.basetype = TYPE_VOID;

        VEC_PUSH(env->symbols, sym, arena);
        return true;
    }

    return false;
}

int32_t *env_get(Environment *env, StringView name) {
    Symbol *entry = env_find(env, name);
    return entry ? &entry->offset : NULL;
}