#ifndef WOD_ERROR_H_
#define WOD_ERROR_H_

#include "lexer.h"

void error(StringView file_path, const char *source, Token *token, StringView message);

#endif