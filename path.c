#include <windows.h>

#include "path.h"

StringView get_full_path(StringView path, Arena *arena) {
    char *dup = sv_dup(arena, path);
    if (!dup) return SV_NULL;
    DWORD size = GetFullPathName(dup, 0, NULL, NULL);

    char *outbuf = arena_alloc(arena, size);
    if (!outbuf) return SV_NULL;
    DWORD ret = GetFullPathName(dup, size, outbuf, NULL);

    return (StringView){ .data = outbuf, .len = ret };
}