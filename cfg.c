#include "cfg.h"

static CFGNode *alloc_node(Arena *arena) {
    CFGNode *node = arena_alloc(arena, sizeof(CFGNode));
    VEC_INIT(node->block);
    node->branch_a = NULL;
    node->branch_b = NULL;

    return node;
}

// Return the node at the _end_ of the path starting from stmt;
// return NULL if path ends in a return.
static CFGNode *visit_Stmt(CFGNode *parent, Stmt *stmt, CFGNode *end, Arena *arena) {
    switch (stmt->type) {
        case NODE_StmtBlock: {
            StmtBlock *s = (StmtBlock *)stmt;
            for (size_t i = 0; i < s->stmts.count; i++) {
                parent = visit_Stmt(parent, s->stmts.at[i], end, arena);
                if (!parent)
                    return NULL;
            }
            return parent;
        }
        case NODE_StmtReturn:
            VEC_PUSH(parent->block, stmt, arena);
            assert(!parent->branch_a);
            parent->branch_a = end;
            return NULL;
        case NODE_StmtIf: {
            VEC_PUSH(parent->block, stmt, arena);
            StmtIf *s = (StmtIf *)stmt;

            assert(!parent->branch_a && !parent->branch_b);
            
            parent->branch_a = alloc_node(arena);
            
            CFGNode *end_of_if_path 
                = visit_Stmt(parent->branch_a, s->then_branch, end, arena);
            
            if (!s->else_branch) {
                CFGNode *join = alloc_node(arena);

                if (end_of_if_path)
                    end_of_if_path->branch_a = join;
                
                parent->branch_b = join;
                return join;
            }

            parent->branch_b = alloc_node(arena);
            CFGNode *end_of_else_path 
                = visit_Stmt(parent->branch_b, s->else_branch, end, arena);

            // If both paths end in a return, it's not possible
            // to go past the if-else statement.
            if (!end_of_if_path && !end_of_else_path)
                return NULL;

            CFGNode *join = alloc_node(arena);
            if (end_of_if_path)
                end_of_if_path->branch_a = join;

            if (end_of_else_path)
                end_of_else_path->branch_a = join;
            
            return join;
        }
        case NODE_StmtLoop:
            UNIMPLEMENTED;
        case NODE_StmtFor:
            UNIMPLEMENTED;
        case NODE_StmtContinue:
            UNIMPLEMENTED;
        case NODE_StmtBreak:
            UNIMPLEMENTED;

        case NODE_StmtSMDecl:
        case NODE_StmtSMState:
            UNREACHABLE;
        default:
            VEC_PUSH(parent->block, stmt, arena);
            return parent;
    }
}

CFGNode *generate_cfg(VEC_PTR_Stmt *func_body, Arena *arena) {
    CFGNode *start = alloc_node(arena);
    StmtBlock fake_block = {
        .base.type = NODE_StmtBlock,
        .stmts = *func_body };
    
    CFGNode *end = alloc_node(arena);

    CFGNode *last = visit_Stmt(start, (Stmt *)&fake_block, end, arena);
    last->branch_a = end;

    return start;
}
#include <stdio.h>

// Ad-hoc dangerous function.
static StringView number_to_sv(size_t a, Arena *arena) {
    char *buf = arena_alloc(arena, 10);
    sprintf(buf, "%zu", a);
    return to_sv(buf);
}

static StringView transition(StringView sv, size_t a, size_t b, Arena *arena) {
    sv = sv_concat(arena, sv, number_to_sv(a, arena));
    sv = sv_concat(arena, sv, SV("->"));
    sv = sv_concat(arena, sv, number_to_sv(b, arena));
    sv = sv_concat(arena, sv, SV(";"));
    return sv;
}

VEC_PTR_DEF(CFGNode);

// Extremely inefficient since it copies the whole string 
// each time you append anything, but it will do for now.
static StringView graph_gen(VEC_PTR_CFGNode *visited, CFGNode *parent, CFGNode *node, StringView sv, Arena *arena) {
    if (!node) return sv;

    if (!node->branch_a) {
        // Node is the end node.
        sv = transition(sv, parent->block.at[0]->loc.line,
            9999, arena);
        return sv;
    }

    if (node->block.count == 0) {
        // Node is some join after a branch, but doesn't have any
        // statements in it. This only happens when it transitions
        // to the end node immediately after.
        // Ideally we'd just prune that node since it does nothing, but
        // uhh pain.
        sv = transition(sv, parent->block.at[0]->loc.line,
            8888, arena);
        return sv;
    }

    sv = transition(sv, parent->block.at[0]->loc.line,
        node->block.at[0]->loc.line, arena);

    for (size_t i = 0; i < visited->count; i++) {
        if (visited->at[i] == node)
            return sv;
    }
    VEC_PUSH(*visited, node, arena);

    sv = graph_gen(visited, node, node->branch_a, sv, arena);
    return graph_gen(visited, node, node->branch_b, sv, arena);    
}

StringView generate_cfg_graph(CFGNode *start, Arena *arena) {
    if (start->block.count <= 0)
        return SV_NULL;

    StringView sv = SV("digraph {");

    VEC_PTR_CFGNode visited;
    VEC_INIT(visited);
    VEC_PUSH(visited, start, arena);

    sv = graph_gen(&visited, start, start->branch_a, sv, arena);
    sv = graph_gen(&visited, start, start->branch_b, sv, arena);

    sv = sv_concat(arena, sv, SV("}\n"));

    return sv;
}