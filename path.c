// Needed for PathIsRelative.
#pragma comment(lib, "Shlwapi")

#include <windows.h>
#include <shlwapi.h>

#include "path.h"

StringView get_full_path(StringView path, Arena *arena) {
    char *dup = sv_dup(arena, path);
    if (!dup) return SV_NULL;
    DWORD size = GetFullPathNameA(dup, 0, NULL, NULL);
    if (size == 0) return SV_NULL;

    char *outbuf = arena_alloc(arena, size * sizeof(TCHAR));
    if (!outbuf) return SV_NULL;
    size = GetFullPathNameA(dup, size, outbuf, NULL);
    if (size == 0) return SV_NULL;

    return (StringView){ .data = outbuf, .len = size };
}

StringView get_directory(StringView path, Arena *arena) {
    StringView full = get_full_path(path, arena);
    if (sv_is_null(full)) return SV_NULL;

    int i = full.len - 1;
    for (; i >= 0; i--) {
        if (full.data[i] == '\\'
            || full.data[i] == '/') break;
    }

    if (i < 0) return SV("");

    return (StringView){ .data = full.data, .len = i + 1 };
}

bool path_is_relative(char *path) {
    return PathIsRelativeA(path);
}