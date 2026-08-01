// Adapted from https://craftinginterpreters.com/hash-tables.html.

#ifndef WOD_TABLE_H_
#define WOD_TABLE_H_

#include "common.h"

#define TABLE_MAX_LOAD 0.75

#define TABLE_DEF(T) \
    typedef struct { \
        StringView key; \
        T value; \
    } ENTRY_##T; \
    \
    typedef struct { \
        size_t capacity; \
        size_t count; \
        ENTRY_##T *at; \
    } TABLE_##T; \
    \
    ENTRY_##T *table_find_##T(TABLE_##T *table, StringView key) { \
        return (ENTRY_##T *)table_find_generic(table, key); \
    }


#define TABLE_EMPTY { .capacity = 0, .count = 0, .at = NULL }

void *table_find_generic(void *table, StringView key);

#endif // WOD_TABLE_H_