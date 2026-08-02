#ifndef WOD_TYPECHECKER_H_
#define WOD_TYPECHECKER_H_

#include "parser.h"
#include "module.h"
#include "environment.h"

// Assumes a topologically-sorted list of modules where the
// last module is the one containing the main() function.
bool typecheck_modules(VEC_Module *modules, Arena *arena);

#endif // WOD_TYPECHECKER_H_