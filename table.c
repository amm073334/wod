#include "table.h"

typedef struct {
    StringView key;
} Entry;

typedef struct {
    size_t capacity;
    size_t count;
    Entry *at;
} Table;

static uint32_t hash(StringView key) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < key.len; i++) {
        hash ^= (uint8_t)key.data[i];
        hash *= 16777619;
    }
    return hash;
}

void *table_find_generic(void *table, StringView key) {
    Table *t = (Table *)table;
    uint32_t index = hash(key) % t->capacity;
    for (;;) {
        Entry* entry = &t->at[index];
        if (sv_is_null(entry->key) || sv_equals(entry->key, key)) {
            return entry;
        }

        index = (index + 1) % t->capacity;
    }
}

static Entry *table_insert_generic(void *table, StringView key, void *value) {
    Table *t = (Table *)table;
    Entry *entry = table_find_generic(table, key);



}