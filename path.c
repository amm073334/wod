#include <windows.h>

#include "path.h"

StringView get_full_path(StringView path, Arena *arena) {
    char *dup = sv_dup(arena, path);
    if (!dup) return SV_NULL;
    DWORD size = GetFullPathName(dup, 0, NULL, NULL);
    if (size == 0) return SV_NULL;

    char *outbuf = arena_alloc(arena, size * sizeof(TCHAR));
    if (!outbuf) return SV_NULL;
    size = GetFullPathName(dup, size, outbuf, NULL);
    if (size == 0) return SV_NULL;

    return (StringView){ .data = outbuf, .len = size };
}