#include "ast2wir.h"
#include "parser.h"
#include "wir.h"

#define ALLOC_WIR(var, type_, ...) \
    do { \
        (var) = arena_alloc_assert(aw->arena, sizeof(type_)); \
        *var = __VA_ARGS__; \
        (var)->base.kind = _##type_; \
    } while (0)

typedef struct {
    size_t int_top;
    size_t str_top;

    // Information about what kind of loop the frame is a part of.
    // This is needed so that a `continue` statement can figure out
    // what to do before it goes back to the top of the loop.
    // If the frame isn't actually a loop, then this information
    // is meaningless.
    enum {
        LOOP_BASIC,
        LOOP_RANGE_INC,
        LOOP_C,
    } loop_kind;
    union {
        Stmt *c_loop_stmt;
        WIROperand range_var;
    };
} Frame;
VEC_DEF(Frame);

typedef struct Ast2Wir {
    Arena *arena;
    WIR *wir;

    VEC_Frame local_frames;
} Ast2Wir;

static Frame *get_current_frame(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    return &aw->local_frames.at[aw->local_frames.count - 1];
}

static WIRInst *get_last_inst(Ast2Wir *aw) {
    WIRCev *cur_cev = &aw->wir->g_cevs.at[aw->wir->g_cevs.count - 1];
    return cur_cev->insts.at[cur_cev->insts.count - 1];
}

static WIROperand get_last_int(Ast2Wir *aw) {
    Frame *last = get_current_frame(aw);
    return (WIROperand){
        .kind = OPKIND_LOCAL_INT,
        .as.offset = last->int_top - 1
    };
}

static void update_prev_inst_dest(Ast2Wir *aw, WIROperand new_dest) {
    assert(aw->wir->g_cevs.count > 0);
    WIRInst *last_inst = get_last_inst(aw);

    switch (last_inst->kind) {
    case _WIRInst_Binop:
        ((WIRInst_Binop *)last_inst)->dest = new_dest;
        break;
    case _WIRInst_Call:
        ((WIRInst_Call *)last_inst)->dest = new_dest;
            break;
    default: UNREACHABLE;
    }
}

static void emit_to_current_cev(Ast2Wir *aw, WIRInst *inst) {
    assert(aw->wir->g_cevs.count > 0);

    WIRCev *last = &aw->wir->g_cevs.at[aw->wir->g_cevs.count - 1];
    VEC_PUSH(last->insts, inst, aw->arena);
}

static void emit_binop(Ast2Wir *aw, WIROperand dest, int assign, WIROperand a, WIROperand b, int op) {
    WIRInst_Binop *inst;
    ALLOC_WIR(inst, WIRInst_Binop,
        (WIRInst_Binop){ .dest = dest, .op = op, .assign = assign, .a = a, .b = b });
    emit_to_current_cev(aw, (WIRInst *)inst);
}

static void emit_compare(Ast2Wir *aw, WIROperand dest, WIROperand a, WIROperand b, int op) {
    WIRInst_Compare *inst;
    ALLOC_WIR(inst, WIRInst_Compare,
        (WIRInst_Compare){ .dest = dest, .op = op, .a = a, .b = b });
    emit_to_current_cev(aw, (WIRInst *)inst);
}

static void emit_if_begin(Ast2Wir *aw, WIROperand cond) {
    WIRInst_IfBegin *inst;
    ALLOC_WIR(inst, WIRInst_IfBegin,
        (WIRInst_IfBegin){ .cond = cond });
    emit_to_current_cev(aw, (WIRInst *)inst);
}

static void emit_str(Ast2Wir *aw, WIROperand dest, WIROperand a) {
    WIRInst_StrAssign *inst;
    ALLOC_WIR(inst, WIRInst_StrAssign,
        (WIRInst_StrAssign){ .dest = dest, .src = a });
    emit_to_current_cev(aw, (WIRInst *)inst);
}

static void emit_return_val(Ast2Wir *aw, WIROperand a) {
    WIRInst_ReturnVal *inst;
    ALLOC_WIR(inst, WIRInst_ReturnVal,
        (WIRInst_ReturnVal){ .val = a });
    emit_to_current_cev(aw, (WIRInst *)inst);
}

static void emit_simple(Ast2Wir *aw, int kind) {
    WIRInst *inst = arena_alloc_assert(aw->arena, sizeof(WIRInst));
    inst->kind = kind;
    emit_to_current_cev(aw, inst);
}

static WIROperand tmp_int(Ast2Wir *aw) {
    WIRCev *cur_cev = &aw->wir->g_cevs.at[aw->wir->g_cevs.count - 1];
    return (WIROperand){
        .kind = OPKIND_TEMP_INT,
        .as.offset = cur_cev->n_temp_ints++ 
    };
}

static WIROperand tmp_str(Ast2Wir *aw) {
    WIRCev *cur_cev = &aw->wir->g_cevs.at[aw->wir->g_cevs.count - 1];
    return (WIROperand){
        .kind = OPKIND_TEMP_STR,
        .as.offset = cur_cev->n_temp_strs++ 
    };
}

static void open_frame(Ast2Wir *aw) {
    if (aw->local_frames.count == 0) {
        VEC_PUSH(aw->local_frames,
            ((Frame){ .int_top = 0, .str_top = 0, .loop_kind = LOOP_BASIC }),
            aw->arena);
    } else {
        Frame *top = get_current_frame(aw);
        VEC_PUSH(aw->local_frames,
            ((Frame){ .int_top = top->int_top, .str_top = top->str_top,
                .loop_kind = LOOP_BASIC}),
            aw->arena);
    }
}

static void close_frame(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    Frame *to_pop = get_current_frame(aw);

    size_t prev_ints;
    size_t prev_strs;
    if (aw->local_frames.count > 1) {
        Frame *prev = &aw->local_frames.at[aw->local_frames.count - 2];
        prev_ints = prev->int_top;
        prev_strs = prev->str_top;
    } else {
        prev_ints = 0;
        prev_strs = 0;
    }
    assert(prev_ints <= to_pop->int_top);
    assert(prev_strs <= to_pop->str_top);

    size_t allocated_ints = to_pop->int_top - prev_ints;
    size_t allocated_strs = to_pop->str_top - prev_strs;

    if (allocated_ints > 0) {
        WIRInst_PopIntN *inst; 
        ALLOC_WIR(inst, WIRInst_PopIntN, (WIRInst_PopIntN){ .n = allocated_ints });
        emit_to_current_cev(aw, (WIRInst *)inst);
    }

    if (allocated_strs > 0) {
        WIRInst_PopStrN *inst; 
        ALLOC_WIR(inst, WIRInst_PopStrN, (WIRInst_PopStrN){ .n = allocated_strs });
        emit_to_current_cev(aw, (WIRInst *)inst);
    }

    VEC_POP(aw->local_frames);
}

static size_t new_local_int(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    emit_simple(aw, _WIRInst_PushInt);
    Frame *top = &aw->local_frames.at[aw->local_frames.count - 1];
    return top->int_top++;
}

static size_t new_local_str(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    emit_simple(aw, _WIRInst_PushStr);
    Frame *top = &aw->local_frames.at[aw->local_frames.count - 1];
    return top->str_top++;
}

static void visit_db_field_decl(Ast2Wir *aw, Stmt *stmt) {
    assert(stmt->kind == NODE_StmtVarDecl);

    StmtVarDecl *s = (StmtVarDecl *)stmt;
    assert(!s->is_const);

    WIRDB *last = &aw->wir->g_cdbs.at[aw->wir->g_cdbs.count - 1];

    WIRVar wdbf = {
        .name = s->name,
        .type = s->sym->type,
        .has_initializer = false
    };

    if (s->initializer) {
        assert(s->initializer->type.is_compile_time);
        wdbf.has_initializer = true;

        switch (s->initializer->kind) {
        case NODE_ExprStrLit: {
            ExprStrLit *lit = (ExprStrLit *)s->initializer;
            wdbf.initializer = (WIROperand){
                .kind = OPKIND_IMM_STR,
                .as.imm_str = lit->value
            };
            break;
        }
        case NODE_ExprIntLit: {
            ExprIntLit *lit = (ExprIntLit *)s->initializer;
            wdbf.initializer = (WIROperand){
                .kind = OPKIND_IMM_INT,
                .as.imm_int = lit->value
            };
            break;
        }
        case NODE_ExprBoolLit: {
            ExprBoolLit *lit = (ExprBoolLit *)s->initializer;
            wdbf.initializer = (WIROperand){
                .kind = OPKIND_IMM_INT,
                .as.imm_int = lit->value
            };
            break;
        }
        default: UNREACHABLE;
        }
    }

    VEC_PUSH(last->fields, wdbf, aw->arena);
}

static WIROperand visit_Expr(Ast2Wir *aw, Expr *expr) {
    switch (expr->kind) {
    case NODE_ExprVar: {
        ExprVar *e = (ExprVar *)expr;

        if (expr->type.is_compile_time) {
            switch (expr->type.basetype) {
            case TYPE_INT: return WIR_IMM_I(e->sym->const_i);
            case TYPE_STR: return WIR_IMM_S(e->sym->const_s);
            case TYPE_BOOL: return WIR_IMM_I(e->sym->const_b);
            default: UNREACHABLE;
            }
        }

        if (e->sym->type.basetype == TYPE_ARRAY)
            UNIMPLEMENTED;

        if (sv_is_null(e->sym->top_level_path)) {
            return (WIROperand){
                .kind = e->sym->type.basetype == TYPE_STR ?
                    OPKIND_LOCAL_STR : OPKIND_LOCAL_INT,
                .as.offset = e->sym->offset 
            };
        } else {
            int kind = OPKIND_GLOBAL_INT;
            switch (e->sym->type.basetype) {
            case TYPE_NONE:
            case TYPE_ERROR:
            case TYPE_VOID:
            case TYPE_MODULE:
                UNREACHABLE;
            case TYPE_INT:
            case TYPE_BOOL:
            case TYPE_PTR:
            case TYPE_DBDATA:
            case TYPE_DBTYPE:
            case TYPE_CEVTYPE:
                kind = OPKIND_GLOBAL_INT;
                break;
            case TYPE_STR:
                kind = OPKIND_GLOBAL_STR;
                break;
            case TYPE_FUNC:
                kind = OPKIND_GLOBAL_CEV;
                break;
            case TYPE_ARRAY:
                UNIMPLEMENTED;
            }

            return (WIROperand){
                .kind = kind,
                .as.global = {
                    .path = e->sym->top_level_path,
                    .name = e->sym->name
            }};
        }
    }
    case NODE_ExprArray: {
        ExprArray *e = (ExprArray *)expr;
        WIROperand index = visit_Expr(aw, e->index);
        visit_Expr(aw, e->left);
        if (e->left->type.basetype == TYPE_DBTYPE)  {
            
        }

        (void) index;

        // TODO: do stuff
        return (WIROperand){0};
    }
    case NODE_ExprAccess: {
        ExprAccess *e = (ExprAccess *)expr;
        (void) e;
        // TODO: do stuff
        return (WIROperand){0};
    }
    case NODE_ExprBinary: {
        ExprBinary *e = (ExprBinary *)expr;

        WIROperand dest = tmp_int(aw);

        // Logical operators use short-circuit evaluation, so they need
        // some special treatment. Otherwise, we could just evaluate both
        // left and right operands up-front and use a binary operator on them to
        // compute the result.
        if (e->op.type == TOK_AMP_AMP) {
            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(0), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            WIROperand left = visit_Expr(aw, e->left);
            emit_if_begin(aw, left);
            
            WIROperand right = visit_Expr(aw, e->right);
            emit_if_begin(aw, right);

            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            emit_simple(aw, _WIRInst_IfEnd);
            emit_simple(aw, _WIRInst_IfEnd);

            return dest;
        } else if (e->op.type == TOK_PIPE_PIPE) {
            // By De Morgan's laws, a || b can be expressed as !(!a && !b).
            // Doing things this way saves us from having to either duplicate code
            // or use a label/goto. (Whether or not that is good performance-wise
            // needs investigation.)

            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            WIROperand left = visit_Expr(aw, e->left);
            WIROperand tmp_left = tmp_int(aw);
            emit_binop(aw, tmp_left, WIR_ASSIGN_EQ, left, WIR_IMM_I(1), WIR_BINOP_XOR);
            emit_if_begin(aw, tmp_left);

            WIROperand right = visit_Expr(aw, e->right);
            WIROperand tmp_right = tmp_int(aw);
            emit_binop(aw, tmp_right, WIR_ASSIGN_EQ, right, WIR_IMM_I(1), WIR_BINOP_XOR);
            emit_if_begin(aw, tmp_right);

            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(0), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            emit_simple(aw, _WIRInst_IfEnd);
            emit_simple(aw, _WIRInst_IfEnd);

            return dest;
        }

        WIROperand left = visit_Expr(aw, e->left);
        WIROperand right = visit_Expr(aw, e->right);

        switch (e->op.type) {
        case TOK_PLUS:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_ADD); break;
        case TOK_MINUS:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_SUB); break;
        case TOK_STAR:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_MUL); break;
        case TOK_SLASH:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_DIV); break;
        case TOK_PERCENT:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_MOD); break;
        case TOK_CARET:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_XOR); break;
        case TOK_PIPE:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_OR); break;
        case TOK_AMP:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_AND); break;
        case TOK_LESS_LESS:
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, right, WIR_BINOP_LSH); break;
        case TOK_GREATER_GREATER: {
            WIROperand negated = tmp_int(aw);
            emit_binop(aw, negated, WIR_ASSIGN_EQ, WIR_IMM_I(0), right, WIR_BINOP_SUB);
            emit_binop(aw, dest, WIR_ASSIGN_EQ, left, negated, WIR_BINOP_LSH);
            break;
        }
        case TOK_EQUAL_EQUAL:
            emit_compare(aw, dest, left, right, WIR_CMP_EQ); break;
        case TOK_BANG_EQUAL:
            emit_compare(aw, dest, left, right, WIR_CMP_NEQ); break;
        case TOK_LESS:
            emit_compare(aw, dest, left, right, WIR_CMP_LT); break;
        case TOK_LESS_EQUAL:
            emit_compare(aw, dest, left, right, WIR_CMP_LTE); break;
        case TOK_GREATER:
            emit_compare(aw, dest, left, right, WIR_CMP_GT); break;
        case TOK_GREATER_EQUAL:
            emit_compare(aw, dest, left, right, WIR_CMP_GTE); break;
        default: UNREACHABLE;
        }

        return dest;
    }
    case NODE_ExprUnary: {
        ExprUnary *e = (ExprUnary *)expr;

        WIROperand right = visit_Expr(aw, e->right);
        
        switch (e->op.type) {
        case TOK_MINUS: {
            WIROperand dest = tmp_int(aw);
            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(0), right, WIR_BINOP_SUB);
            return dest;
        }
        case TOK_BANG: {
            // Assumes that booleans are either zero or one.
            WIROperand dest = tmp_int(aw);
            emit_binop(aw, dest, WIR_ASSIGN_EQ, right, WIR_IMM_I(1), WIR_BINOP_XOR);
            return dest;
        }
        case TOK_AMP: {
            assert(op_is_local(right) || op_is_global(right));
            return right;
        }
        default: UNREACHABLE;
        }

        return (WIROperand){ 0 };
    }
    case NODE_ExprCall: {
        ExprCall *e = (ExprCall *)expr;

        WIROperand callee = visit_Expr(aw, e->callee);
        VEC_WIROperand args = VEC_EMPTY;
        for (size_t i = 0; i < e->args.count; i++) {
            VEC_PUSH(args,
                visit_Expr(aw, e->args.at[i]), aw->arena);
        }

        WIROperand dest;
        if (e->base.type.basetype == TYPE_STR)
            dest = tmp_str(aw);
        else
            dest = tmp_int(aw);

        WIRInst_Call *inst;
        ALLOC_WIR(inst, WIRInst_Call,
            (WIRInst_Call){.dest = dest, .cev = callee, .args = args});
        emit_to_current_cev(aw, (WIRInst *)inst);

        return dest;
    }

    case NODE_ExprIntLit: {
        ExprIntLit *e = (ExprIntLit *)expr;
        return WIR_IMM_I(e->value);
    }
    case NODE_ExprStrLit: {
        ExprStrLit *e = (ExprStrLit *)expr;
        return WIR_IMM_S(e->value);
    }
    case NODE_ExprBoolLit: {
        ExprBoolLit *e = (ExprBoolLit *)expr;
        return WIR_IMM_I(e->value);
    }
    case NODE_ExprInterp: {
        VEC_WIROperand results = VEC_EMPTY;

        Expr *p = expr;
        while (p->kind == NODE_ExprInterp) {
            ExprInterp *e = (ExprInterp *)p;
            VEC_PUSH(results, WIR_IMM_S(e->opening), aw->arena);
            VEC_PUSH(results, visit_Expr(aw, e->expr), aw->arena);
            p = e->next;
        }
        assert(p->kind == NODE_ExprStrLit);
        VEC_PUSH(results, visit_Expr(aw, p), aw->arena);

        return (WIROperand){
            .kind = OPKIND_INTERP,
            .as.interp = results
        };
    }
    }

    UNREACHABLE;
    return (WIROperand){ 0 };
}

static void visit_Stmt(Ast2Wir *aw, Stmt *stmt) {
    switch (stmt->kind) {
    case NODE_StmtAssign: {
        StmtAssign *s = (StmtAssign *)stmt;
        WIROperand left = visit_Expr(aw, s->left);
        WIROperand right = visit_Expr(aw, s->right);

        if (op_is_string(right)) {
            assert(op_is_string(left));
            emit_str(aw, left, right);
        } else {
            switch (s->assign_type.type) {
            case TOK_EQUAL:
                emit_binop(aw, left, WIR_ASSIGN_EQ, right, WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case TOK_PLUS_EQUAL:
                emit_binop(aw, left, WIR_ASSIGN_ADD, right, WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case TOK_MINUS_EQUAL:
                emit_binop(aw, left, WIR_ASSIGN_SUB, right, WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case TOK_STAR_EQUAL:
                emit_binop(aw, left, WIR_ASSIGN_MUL, right, WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case TOK_SLASH_EQUAL:
                emit_binop(aw, left, WIR_ASSIGN_DIV, right, WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case TOK_PERCENT_EQUAL:
                emit_binop(aw, left, WIR_ASSIGN_MOD, right, WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case TOK_AMP_EQUAL: {
                emit_binop(aw, left, WIR_ASSIGN_EQ, left, right, WIR_BINOP_AND);
                break;
            }
            case TOK_PIPE_EQUAL: {
                emit_binop(aw, left, WIR_ASSIGN_EQ, left, right, WIR_BINOP_OR);
                break;
            }
            default: UNREACHABLE;
            }
        }
        return;
    }
    case NODE_StmtVarDecl: {
        StmtVarDecl *s = (StmtVarDecl *)stmt;

        if (s->array_length) {
            visit_Expr(aw, s->array_length);
        }

        switch (s->sym->type.basetype) {
        case TYPE_STR: {
            s->sym->offset = new_local_str(aw);

            WIROperand init = { 0 };
            if (s->initializer) {
                init = visit_Expr(aw, s->initializer);
                emit_str(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL_STR,
                        .as.offset = s->sym->offset
                    },
                    init);
            }

            if (!s->sym->env->parent) {
                WIRVar wv = {
                    .name = s->sym->name,
                    .type = s->sym->type,
                    .has_initializer = s->initializer != NULL,
                    .initializer = init
                };
                VEC_PUSH(aw->wir->g_strs, wv, aw->arena);
            }

            break;
        }
        default: {
            s->sym->offset = new_local_int(aw);

            WIROperand init = { 0 };
            if (s->initializer) {
                init = visit_Expr(aw, s->initializer);
                emit_binop(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL_INT,
                        .as.offset = s->sym->offset
                    },
                    WIR_ASSIGN_EQ, init, WIR_IMM_I(0), WIR_BINOP_ADD);
            }

            if (!s->sym->env->parent) {
                WIRVar wv = {
                    .name = s->sym->name,
                    .type = s->sym->type,
                    .has_initializer = s->initializer != NULL,
                    .initializer = init
                };
                VEC_PUSH(aw->wir->g_ints, wv, aw->arena);
            }
            break;
        }
        }
        return;
    }
    case NODE_StmtFuncDecl: {
        StmtFuncDecl *s = (StmtFuncDecl *)stmt;
        
        s->sym->offset = aw->wir->g_cevs.count;
        VEC_PUSH(aw->wir->g_cevs,
            ((WIRCev){
                .name = s->name,
                .insts = VEC_EMPTY,
                .n_temp_ints = 0,
            }), aw->arena);

        open_frame(aw);
        for (size_t i = 0; i < s->body.count; i++)
            visit_Stmt(aw, s->body.at[i]);
        close_frame(aw);
        return;
    }
    case NODE_StmtBlock: {
        StmtBlock *s = (StmtBlock *)stmt;
        open_frame(aw);
        for (size_t i = 0; i < s->stmts.count; i++)
            visit_Stmt(aw, s->stmts.at[i]);
        close_frame(aw);
        return;
    }
    case NODE_StmtReturn: {
        StmtReturn *s = (StmtReturn *)stmt;

        if (s->expr) {
            WIROperand ret = visit_Expr(aw, s->expr);
            emit_return_val(aw, ret);
        } else {
            emit_simple(aw, _WIRInst_ReturnVoid);
        }

        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        WIROperand cond = visit_Expr(aw, s->condition);

        emit_if_begin(aw, cond);

        visit_Stmt(aw, s->then_branch);

        if (s->else_branch) {
            emit_simple(aw, _WIRInst_Else);
            visit_Stmt(aw, s->else_branch);
        }

        emit_simple(aw, _WIRInst_IfEnd);

        return;
    }
    case NODE_StmtLoop: {
        StmtLoop *s = (StmtLoop *)stmt;

        if (s->count) {
            WIROperand count = visit_Expr(aw, s->count);

            WIRInst_LoopBeginN *inst;
            ALLOC_WIR(inst, WIRInst_LoopBeginN,
                (WIRInst_LoopBeginN){ .count = count });
            emit_to_current_cev(aw, (WIRInst *)inst);
        } else {
            emit_simple(aw, _WIRInst_LoopBegin);
        }
        visit_Stmt(aw, s->body);

        emit_simple(aw, _WIRInst_LoopEnd);
        return;
    }
    case NODE_StmtForC: {
        StmtForC *s = (StmtForC *)stmt;
        open_frame(aw);

        if (s->iter_stmt) {
            Frame *frame = get_current_frame(aw);
            frame->loop_kind = LOOP_C;
            frame->c_loop_stmt = s->iter_stmt;
        }

        if (s->init)
            visit_Stmt(aw, s->init);
        
        emit_simple(aw, _WIRInst_LoopBegin);
        
        if (s->condition) {
            WIROperand cond = visit_Expr(aw, s->condition);
            WIROperand tmp_cond = tmp_int(aw);
            emit_binop(aw, tmp_cond, WIR_ASSIGN_EQ, cond, WIR_IMM_I(1), WIR_BINOP_XOR);

            WIRInst_IfBegin *if_begin;
            ALLOC_WIR(if_begin, WIRInst_IfBegin,
                (WIRInst_IfBegin){ .cond = tmp_cond });

            emit_to_current_cev(aw, (WIRInst *)if_begin);
            emit_simple(aw, _WIRInst_Break);
            emit_simple(aw, _WIRInst_IfEnd);
        }
            
        visit_Stmt(aw, s->body);

        if (s->iter_stmt)
            visit_Stmt(aw, s->iter_stmt);

        emit_simple(aw, _WIRInst_LoopEnd);
        close_frame(aw);
        return;
    }
    case NODE_StmtForRange: {
        StmtForRange *s = (StmtForRange *)stmt;
        open_frame(aw);

        visit_Stmt(aw, (Stmt *)s->decl);
        WIROperand iterator = get_last_int(aw);

        Frame *frame = get_current_frame(aw);
        frame->loop_kind = LOOP_RANGE_INC;
        frame->range_var = iterator;

        WIROperand right_bound = visit_Expr(aw, s->right_bound);
        WIROperand loop_count = tmp_int(aw);
        emit_binop(aw, loop_count, WIR_ASSIGN_EQ, right_bound, iterator, WIR_BINOP_SUB);
        
        WIRInst_LoopBeginN *loop_begin;
        ALLOC_WIR(loop_begin, WIRInst_LoopBeginN,
            (WIRInst_LoopBeginN){ .count = loop_count });

        emit_to_current_cev(aw, (WIRInst *)loop_begin);

        visit_Stmt(aw, s->body);

        emit_binop(aw, iterator, WIR_ASSIGN_ADD,
            WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
        emit_simple(aw, _WIRInst_LoopEnd);
        close_frame(aw);
        return;
    }
    case NODE_StmtContinue: {
        Frame *top = get_current_frame(aw);
        switch (top->loop_kind) {
            case LOOP_BASIC: break;
            case LOOP_RANGE_INC:
                emit_binop(aw, top->range_var, WIR_ASSIGN_ADD,
                    WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
                break;
            case LOOP_C:
                visit_Stmt(aw, top->c_loop_stmt);
                break;
        }
        emit_simple(aw, _WIRInst_Continue);
        return;
    }
    case NODE_StmtBreak: 
        emit_simple(aw, _WIRInst_Break);
        return;
    case NODE_StmtCmd: {
        StmtCmd *s = (StmtCmd *)stmt;

        VEC_WIROperand iargs = VEC_EMPTY;
        VEC_WIROperand sargs = VEC_EMPTY;

        assert(s->id->kind == NODE_ExprIntLit);
        int32_t cmd_id = ((ExprIntLit *)s->id)->value;

        for (size_t i = 0; i < s->int_operands.count; i++)
            VEC_PUSH(iargs, visit_Expr(aw, s->int_operands.at[i]), aw->arena);
        for (size_t i = 0; i < s->str_operands.count; i++)
            VEC_PUSH(sargs, visit_Expr(aw, s->str_operands.at[i]), aw->arena);

        // TODO: Currently, let all relative indent values be 0.
        WIRInst_Cmd *inst;
        ALLOC_WIR(inst, WIRInst_Cmd, (WIRInst_Cmd){
            .op = cmd_id, .open_close = 0, .iargs = iargs, .sargs = sargs});

        emit_to_current_cev(aw, (WIRInst *)inst);
        return;
    }
    case NODE_StmtCall: {
        StmtCall *s = (StmtCall *)stmt;
        visit_Expr(aw, (Expr *)s->call);

        // Calls used as an expression statement have no destination:
        // Therefore they can just have a 0 destination to save
        // a temporary.
        WIRInst *last = get_last_inst(aw);
        ((WIRInst_Call *)last)->dest = (WIROperand){
            .kind = OPKIND_IMM_INT,
            .as.imm_int = 0
        };

        return;
    }
    case NODE_StmtDBDecl: {
        StmtDBDecl *s = (StmtDBDecl *)stmt;
        VEC_PUSH(aw->wir->g_cdbs,
            ((WIRDB){
                .name = s->name,
                .fields = VEC_EMPTY
            }), aw->arena);
        for (size_t i = 0; i < s->fields.count; i++)
            visit_db_field_decl(aw, (Stmt *)s->fields.at[i]);
        return;
    }
    case NODE_StmtInc: {
        StmtInc *s = (StmtInc *)stmt;
        WIROperand left = visit_Expr(aw, s->expr);
        emit_binop(aw, left, WIR_ASSIGN_ADD, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
        return;
    }
    case NODE_StmtDec: {
        StmtDec *s = (StmtDec *)stmt;
        WIROperand left = visit_Expr(aw, s->expr);
        emit_binop(aw, left, WIR_ASSIGN_SUB, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
        return;
    }
    }
}

void ast2wir_pass(VEC_Module *modules, Arena *arena) {
    for (size_t i = 0; i < modules->count; i++) {
        Ast2Wir aw = (Ast2Wir){
            .arena = arena,
            .wir = arena_alloc_assert(arena, sizeof(WIR)),
            .local_frames = VEC_EMPTY,
        };
        wir_init(aw.wir);

        Module *mod = &modules->at[i];
        for (size_t j = 0; j < mod->ast->stmts.count; j++) {
            visit_Stmt(&aw, mod->ast->stmts.at[j]);
            assert(aw.local_frames.count == 0);
        }

        mod->wir = aw.wir;
    }
}