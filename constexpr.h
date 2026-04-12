// Handle evaluation of constants and constant expressions.

#ifndef WOD_CONSTEXPR_H_
#define WOD_CONSTEXPR_H_

#include "common.h"
#include "parser.h"

void constexpr_pass(ProgramAST *ast, Arena *arena);

#endif // WOD_CONSTEXPR_H_