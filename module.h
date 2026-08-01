#ifndef WOD_MODULE_H_
#define WOD_MODULE_H_

#include "common.h"
#include "source.h"
#include "parser.h"
#include "environment.h"
#include "wir.h"

typedef struct Module {
    Source *source;
    ProgramAST *ast;
    Environment *env;
    WIR *wir;
} Module;
VEC_DEF(Module);

#endif // WOD_MODULE_H_