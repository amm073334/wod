#ifndef WOD_TYPECHECKER_H_
#define WOD_TYPECHECKER_H_

#include "parser.h"
#include "environment.h"

Environment *typecheck(ProgramAST *ast, const char *source, Arena *arena);

#endif // WOD_TYPECHECKER_H_