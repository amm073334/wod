#include "environment.h"
#include <stdio.h>

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

// TODO: This would be more efficient with a hash table.
Symbol *env_find(Environment *env, StringView name) {
    for (size_t i = 0; i < env->symbols.count; i++) {
        if (!sv_equals(name, env->symbols.at[i].name))
            continue;

        return &env->symbols.at[i];
    }
    return NULL;
}

Symbol *env_insert(Environment *env, StringView name, WodType type, Arena *arena) {
    Symbol *entry = env_find(env, name);
    if (entry) return NULL;

    Symbol sym;
    sym.env = env;
    sym.name = name;
    sym.type = type;
    sym.local_offset = 0;
    sym.top_level_path = SV_NULL;

    VEC_PUSH(env->symbols, sym, arena);
    return &env->symbols.at[env->symbols.count - 1];
}


Symbol *env_find_recursive(Environment *env, StringView name) {
    if (!env) return NULL;
    
    Symbol *sym = env_find(env, name);
    if (sym) return sym;

    return env_find_recursive(env->parent, name);
}