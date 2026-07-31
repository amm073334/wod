#ifndef WOD_TYPECHECKER_H_
#define WOD_TYPECHECKER_H_

#include "parser.h"
#include "environment.h"

VEC_PTR_Environment typecheck_asts(VEC_PTR_ProgramAST asts, Arena *arena);

#endif // WOD_TYPECHECKER_H_