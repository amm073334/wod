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

    env->parent = parent;
    env->symbols.count = 0;
    env->symbols.capacity = 0;
    env->symbols.at = NULL;

    return env;
}

Symbol *env_insert(Environment *env, StringView name, WodType type, size_t offset, Arena *arena) {
    Symbol *entry = env_find(env, name);
    
    if (entry) return NULL;

    Symbol sym;
    sym.name = name;
    sym.type = type;
    sym.offset = offset;

    VEC_PUSH(env->symbols, sym, arena);
    return &env->symbols.at[env->symbols.count - 1];
}

// TODO: This would be more efficient with a hash table.
Symbol *env_find(Environment *env, StringView name) {
    if (!env) return NULL;
    
    for (size_t i = 0; i < env->symbols.count; i++) {
        if (!sv_equals(name, env->symbols.at[i].name))
            continue;

        return &env->symbols.at[i];
    }

    return env_find(env->parent, name);
}