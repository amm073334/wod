#ifndef WOD_PATH_H_
#define WOD_PATH_H_

#include "common.h"

StringView get_full_path(StringView path, Arena *arena);
StringView get_directory(StringView path, Arena *arena);
bool path_is_relative(char *path);

#endif