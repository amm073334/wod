#ifndef WOD_AST2WIR_H_
#define WOD_AST2WIR_H_

#include "module.h"
#include "wir.h"

void ast2wir_pass(VEC_Module *modules, Arena *arena);

#endif // WOD_AST2WIR_H_