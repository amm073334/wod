#ifndef WOD_ERROR_H_
#define WOD_ERROR_H_

#include "lexer.h"

void error(Location loc, size_t len, StringView message);

#endif // WOD_ERROR_H_