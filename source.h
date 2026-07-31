#ifndef WOD_SOURCE_H_
#define WOD_SOURCE_H_

#include "common.h"

typedef struct Source {
    StringView text;
    StringView path;
} Source;

Source *alloc_source(StringView path, Arena *arena);

#endif // WOD_SOURCE_H_