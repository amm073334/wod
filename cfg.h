#ifndef WOD_CFG_H_
#define WOD_CFG_H_

#include "parser.h"

typedef struct CFGNode CFGNode;
struct CFGNode {
    VEC_PTR_Stmt block;

    // Corresponds to when an if condition is true,
    // or when you go into a loop.
    CFGNode *branch_a;

    // Corresponds to not taking a condition;
    // can be null if there is only one path.
    CFGNode *branch_b;
};

CFGNode *generate_cfg(VEC_PTR_Stmt *func_body, Arena *arena);
StringView generate_cfg_graph(CFGNode *start, Arena *arena);

#endif // WOD_CFG_H_