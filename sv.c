#include "common.h"
#include "sv.h"

StringView to_sv(const char *s) {
    return (StringView){ .data = s, .len = strlen(s) };
}

bool sv_equals(StringView a, StringView b) {
    if (a.len != b.len)
        return false;

    for (size_t i = 0; i < a.len; i++) {
        if (a.data[i] != b.data[i])
            return false;
    }

    return true;
}

bool sv_is_null(StringView s) {
    return s.data == NULL;
}

StringView sv_concat(Arena *arena, StringView a, StringView b) {
    char *new_str = arena_alloc(arena, a.len + b.len + 1);
    if (!new_str) return SV_NULL;
    memcpy(new_str, a.data, a.len);
    memcpy(new_str + a.len, b.data, b.len);
    new_str[a.len + b.len] = '\0';
    return (StringView){.data = new_str, .len = a.len + b.len};
}

char *sv_dup(Arena *arena, StringView s) {
    char *new_str = arena_alloc(arena, s.len + 1);
    if (!new_str) return NULL;
    memcpy(new_str, s.data, s.len);
    new_str[s.len] = '\0';
    return new_str;
}