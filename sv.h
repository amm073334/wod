#ifndef WOD_SV_H_
#define WOD_SV_H_

#include <string.h>

#include "memory.h"

#define SV(s) (StringView){ .data = s, .len = sizeof(s) - 1 }

#define SV_FMT "%.*s"
#define SV_FMT_VAL(sv) (int)(sv).len, (sv).data 

typedef struct {
    const char *data;
    size_t len;
} StringView;

StringView to_sv(const char *s);
bool sv_equals(StringView a, StringView b);
StringView sv_concat(Arena *arena, StringView a, StringView b);

#endif // WOD_SV_H_