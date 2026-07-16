#ifndef WOD_SV_H_
#define WOD_SV_H_

#include <string.h>

#include "memory.h"

#define SV(s) (StringView){ .data = s, .len = sizeof(s) - 1 }
#define SV_NULL (StringView){ .data = NULL }

#define SV_FMT "%.*s"
#define SV_FMT_VAL(sv) (int)(sv).len, (sv).data 

typedef struct {
    const char *data;
    size_t len;
} StringView;

StringView to_sv(const char *s);
bool sv_equals(StringView a, StringView b);

bool sv_is_null(StringView s);
StringView sv_concat(Arena *arena, StringView a, StringView b);
char *sv_dup(Arena *arena, StringView s);
bool sv_to_int(Arena *arena, StringView s, int32_t *out);

#endif // WOD_SV_H_