#ifndef WOD_AST2WIR_H_
#define WOD_AST2WIR_H_

#include "parser.h"
#include "wir.h"

WIR ast2wir_pass(ProgramAST *ast, Arena *arena);

#endif // WOD_AST2WIR_H_