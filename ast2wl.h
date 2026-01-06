#ifndef WOD_AST2WL_H_
#define WOD_AST2WL_H_

#include "parser.h"
#include "wl.h"

VEC_WLInst *generate_wl(VEC_PTR_Stmt *ast, Arena *arena);

#endif