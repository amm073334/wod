#ifndef WOD_TYPECHECKER_H_
#define WOD_TYPECHECKER_H_

#include "parser.h"
#include "environment.h"

Environment *typecheck(VEC_PTR_Stmt *ast, const char *source, Arena *arena);

#endif // WOD_TYPECHECKER_H_