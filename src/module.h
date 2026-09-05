#ifndef WOD_MODULE_H_
#define WOD_MODULE_H_

typedef struct Source Source;
typedef struct ProgramAST ProgramAST;
typedef struct Environment Environment;
typedef struct WIR WIR;

typedef struct Module {
    Source *source;
    ProgramAST *ast;
    Environment *env;
} Module;
VEC_DEF(Module);

#endif // WOD_MODULE_H_