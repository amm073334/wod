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

StringView get_directory(StringView path, Arena *arena) {
    StringView full = get_full_path(path, arena);
    if (full.len == 0) return full;

    int i = full.len - 1;
    for (; i >= 0; i--) {
        if (full.data[i] == '\\'
            || full.data[i] == '/') break;
    }

    if (i < 0) return SV("");

    return (StringView){ .data = full.data, .len = i + 1 };
}