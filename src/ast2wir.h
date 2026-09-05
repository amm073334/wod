#ifndef WOD_AST2WIR_H_
#define WOD_AST2WIR_H_

#include "common.h"
#include "module.h"
#include "wir.h"

WIR ast2wir_pass(VEC_Module *modules, Arena *arena);

#endif // WOD_AST2WIR_H_