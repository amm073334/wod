#include "ast2wir.h"
#include "parser.h"
#include "wir.h"

typedef struct {
    size_t int_top;
    size_t str_top;
} TwoStack;

VEC_DEF(TwoStack);

typedef struct {
    Arena *arena;
    WIR *wir;

    TwoStack tmp;
    TwoStack global;
    VEC_TwoStack local_frames;
} Ast2Wir;

static void emit_to_current_cev(Ast2Wir *aw, WIRInst inst) {
    assert(aw->wir->cevs.count > 0);

    WIRCev *last = &aw->wir->cevs.at[aw->wir->cevs.count - 1];
    VEC_PUSH(last->insts, inst, aw->arena);
}

static void emit_binop(Ast2Wir *aw, WIROperand dest, WIROperand a, WIROperand b, int op) {
    WIRInst inst = (WIRInst) {
        .op = op,
        .operands = VEC_EMPTY
    };

    VEC_PUSH(inst.operands, dest, aw->arena);
    VEC_PUSH(inst.operands, a, aw->arena);
    VEC_PUSH(inst.operands, b, aw->arena);

    emit_to_current_cev(aw, inst);
}

static void emit_str(Ast2Wir *aw, WIROperand dest, WIROperand a) {
    WIRInst inst = (WIRInst) {
        .op = WIR_STR_ASSIGN,
        .operands = VEC_EMPTY
    };

    VEC_PUSH(inst.operands, dest, aw->arena);
    VEC_PUSH(inst.operands, a, aw->arena);

    emit_to_current_cev(aw, inst);
}

static void emit_return_val(Ast2Wir *aw, WIROperand a) {
    WIRInst inst = (WIRInst) {
        .op = WIR_RETURN_VAL,
        .operands = VEC_EMPTY
    };

    VEC_PUSH(inst.operands, a, aw->arena);
    emit_to_current_cev(aw, inst);
}

static void emit_simple(Ast2Wir *aw, int op) {
    emit_to_current_cev(aw, (WIRInst){ .op = op, .operands = VEC_EMPTY });
}

static void emit_single(Ast2Wir *aw, int op, WIROperand operand) {
    VEC_WIROperand operands = VEC_EMPTY;
    VEC_PUSH(operands, operand, aw->arena);
    emit_to_current_cev(aw, (WIRInst){ .op = op, .operands = operands });
}

static WIROperand tmp_int(Ast2Wir *aw) {
    return (WIROperand){
        .kind = OPKIND_TMP, .type = OPTYPE_INT,
        .as.offset = aw->tmp.int_top++ };
}

static WIROperand tmp_str(Ast2Wir *aw) {
    return (WIROperand){
        .kind = OPKIND_TMP, .type = OPTYPE_STR,
        .as.offset = aw->tmp.str_top++ };
}

static void open_frame(Ast2Wir *aw) {
    if (aw->local_frames.count == 0) {
        VEC_PUSH(aw->local_frames,
            ((TwoStack){.int_top = 0, .str_top = 0}),
            aw->arena);
    } else {
        TwoStack top = aw->local_frames.at[aw->local_frames.count - 1];
        VEC_PUSH(aw->local_frames,
            ((TwoStack){.int_top = top.int_top, .str_top = top.str_top}),
            aw->arena);
    }
}

static void close_frame(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    VEC_POP(aw->local_frames);
}

static size_t new_local_int(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    TwoStack *top = &aw->local_frames.at[aw->local_frames.count - 1];
    return top->int_top++;
}

static size_t new_local_str(Ast2Wir *aw) {
    assert(aw->local_frames.count > 0);
    TwoStack *top = &aw->local_frames.at[aw->local_frames.count - 1];
    return top->str_top++;
}

static void visit_db_field_decl(Ast2Wir *aw, Stmt *stmt) {
    assert(stmt->kind == NODE_StmtVarDecl);

    StmtVarDecl *s = (StmtVarDecl *)stmt;
    assert(!s->is_const);

    WIRDB *last = &aw->wir->cdb_types.at[aw->wir->cdb_types.count - 1];

    Symbol *sym = env_find_recursive(stmt->env, s->name);
    assert(sym);

    WIRDBField wdbf = {
        .debug_name = s->name,
        .type = sym->type,
        .has_initializer = false
    };

    if (s->initializer) {
        assert(s->initializer->type.is_compile_time);
        wdbf.has_initializer = true;

        switch (s->initializer->kind) {
        case NODE_ExprStrLit: {
            ExprStrLit *lit = (ExprStrLit *)s->initializer;
            wdbf.initializer = (WIROperand){
                .kind = OPKIND_IMM,
                .type = OPTYPE_STR,
                .as.imm_str = lit->value
            };
            break;
        }
        case NODE_ExprIntLit: {
            ExprIntLit *lit = (ExprIntLit *)s->initializer;
            wdbf.initializer = (WIROperand){
                .kind = OPKIND_IMM,
                .type = OPTYPE_INT,
                .as.imm_int = lit->value
            };
            break;
        }
        case NODE_ExprBoolLit: {
            ExprBoolLit *lit = (ExprBoolLit *)s->initializer;
            wdbf.initializer = (WIROperand){
                .kind = OPKIND_IMM,
                .type = OPTYPE_INT,
                .as.imm_int = lit->value
            };
            break;
        }
        default: UNREACHABLE;
        }
    }

    VEC_PUSH(last->fields, wdbf, aw->arena);
}

static WIROperand visit_Expr(Ast2Wir *aw, Expr *expr);

static WIROperand _visit_Expr(Ast2Wir *aw, Expr *expr) {
    switch (expr->kind) {
    case NODE_ExprVar: {
        ExprVar *e = (ExprVar *)expr;
        Symbol *sym = env_find_recursive(expr->env, e->name);
        assert(sym);
        
        if (expr->type.is_compile_time) {
            switch (expr->type.basetype) {
            case TYPE_INT: return WIR_IMM_I(sym->const_i);
            case TYPE_STR: return WIR_IMM_S(sym->const_s);
            case TYPE_BOOL: return WIR_IMM_I(sym->const_b);
            default: UNREACHABLE;
            }
        }

        if (sv_is_null(sym->top_level_path)) {
            return (WIROperand){
                .kind = OPKIND_LOCAL,
                .as.offset = sym->offset };
        } else {
            return (WIROperand){
                .kind = OPKIND_GLOBAL,
                .as.top.path = sym->top_level_path,
                .as.top.name = sym->name };
        }
    }
    case NODE_ExprArray: {
        ExprArray *e = (ExprArray *)expr;
        WIROperand index = visit_Expr(aw, e->index);
        visit_Expr(aw, e->left);
        if (e->left->type.basetype == TYPE_DBTYPE)  {
            
        }

        // TODO: do stuff
        return (WIROperand){0};
    }
    case NODE_ExprAccess: {
        ExprAccess *e = (ExprAccess *)expr;
        // TODO: do stuff
        return (WIROperand){0};
    }
    case NODE_ExprBinary: {
        ExprBinary *e = (ExprBinary *)expr;

        WIROperand dest = tmp_int(aw);
        WIROperand left = visit_Expr(aw, e->left);
        WIROperand right = visit_Expr(aw, e->right);

        switch (e->op.type) {
        case TOK_PLUS:
            emit_binop(aw, dest, left, right, WIR_ADD); break;
        case TOK_MINUS:
            emit_binop(aw, dest, left, right, WIR_SUB); break;
        case TOK_STAR:
            emit_binop(aw, dest, left, right, WIR_MUL); break;
        case TOK_SLASH:
            emit_binop(aw, dest, left, right, WIR_DIV); break;
        case TOK_PERCENT:
            emit_binop(aw, dest, left, right, WIR_MOD); break;
        case TOK_LESS_LESS:
            emit_binop(aw, dest, left, right, WIR_LSH); break;
        case TOK_GREATER_GREATER: {
            WIROperand negated = tmp_int(aw);
            emit_binop(aw, negated, WIR_IMM_I(0), right, WIR_SUB);
            emit_binop(aw, dest, left, negated, WIR_LSH);
            break;
        }

        case TOK_EQUAL_EQUAL:
            emit_binop(aw, dest, left, right, WIR_EQ); break;
        case TOK_BANG_EQUAL:
            emit_binop(aw, dest, left, right, WIR_NEQ); break;
        case TOK_LESS:
            emit_binop(aw, dest, left, right, WIR_LT); break;
        case TOK_LESS_EQUAL:
            emit_binop(aw, dest, left, right, WIR_LTE); break;
        case TOK_GREATER:
            emit_binop(aw, dest, left, right, WIR_GT); break;
        case TOK_GREATER_EQUAL:
            emit_binop(aw, dest, left, right, WIR_GTE); break;
        case TOK_AMP_AMP:
            emit_binop(aw, dest, left, right, WIR_LAND); break;
        case TOK_PIPE_PIPE:
            emit_binop(aw, dest, left, right, WIR_LOR); break;
        default: UNREACHABLE;
        }

        return dest;
    }
    case NODE_ExprUnary: {
        ExprUnary *e = (ExprUnary *)expr;

        WIROperand dest = tmp_int(aw);
        WIROperand right = visit_Expr(aw, e->right);
        
        switch (e->op.type) {
        case TOK_MINUS:
            emit_binop(aw, dest, WIR_IMM_I(0), right, WIR_SUB); break;
        case TOK_BANG:
            // Assumes that booleans are either zero or one.
            emit_binop(aw, dest, WIR_IMM_I(1), right, WIR_XOR); break;
        default: UNREACHABLE;
        }

        return dest;
    }
    case NODE_ExprCall: {
        ExprCall *e = (ExprCall *)expr;

        WIRInst inst = (WIRInst) {
            .op = WIR_CALL,
            .operands = VEC_EMPTY
        };

        WIROperand dest = tmp_int(aw);
        VEC_PUSH(inst.operands, dest, aw->arena);

        WIROperand callee = visit_Expr(aw, e->callee);
        VEC_PUSH(inst.operands, callee, aw->arena);
        for (size_t i = 0; i < e->args.count; i++) {
            VEC_PUSH(inst.operands,
                visit_Expr(aw, e->args.at[i]), aw->arena);
        }

        emit_to_current_cev(aw, inst);

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
    }

    UNREACHABLE;
    return (WIROperand){ 0 };
}

static WIROperand visit_Expr(Ast2Wir *aw, Expr *expr) {
    TwoStack top = aw->tmp;
    WIROperand operand = _visit_Expr(aw, expr);
    aw->tmp = top;

    // The temporary result of a calculation needs a slot to hold it.
    if (operand.kind == OPKIND_TMP) {
        switch (operand.type) {
        case OPTYPE_INT: aw->tmp.int_top++; break;
        case OPTYPE_STR: aw->tmp.str_top++; break;
        }
    }

    return operand;
}

static void visit_Stmt(Ast2Wir *aw, Stmt *stmt) {
    // Reset temp buffer before every statement.
    aw->tmp = (TwoStack){ 0 };

    switch (stmt->kind) {
    case NODE_StmtAssign: {
        StmtAssign *s = (StmtAssign *)stmt;
        WIROperand left = visit_Expr(aw, s->left);
        WIROperand right = visit_Expr(aw, s->right);

        emit_binop(aw, left, right, WIR_IMM_I(0), WIR_ADD);
        return;
    }
    case NODE_StmtVarDecl: {
        StmtVarDecl *s = (StmtVarDecl *)stmt;

        if (s->array_length) {
            visit_Expr(aw, s->array_length);
        }

        Symbol *sym = env_find_recursive(stmt->env, s->name);
        assert(sym);

        switch (sym->type.basetype) {
        case TYPE_STR:
            sym->offset = new_local_str(aw);

            if (s->initializer) {
                WIROperand init = visit_Expr(aw, s->initializer);
                emit_str(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL,
                        .type = OPTYPE_STR,
                        .as.offset = sym->offset},
                    init);
            }
            break;
        default:
            sym->offset = new_local_int(aw);

            if (s->initializer) {
                WIROperand init = visit_Expr(aw, s->initializer);
                emit_binop(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL,
                        .type = OPTYPE_INT,
                        .as.offset = sym->offset},
                    init, WIR_IMM_I(0), WIR_ADD);
            }
            break;
        }
        return;
    }
    case NODE_StmtFuncDecl: {
        StmtFuncDecl *s = (StmtFuncDecl *)stmt;
        
        Symbol *sym = env_find_recursive(stmt->env, s->name);
        assert(sym);

        sym->offset = aw->wir->cevs.count;
        VEC_PUSH(aw->wir->cevs,
            ((WIRCev){
                .debug_name = s->name,
                .insts = VEC_EMPTY,
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
            emit_simple(aw, WIR_RETURN_VOID);
        }

        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        WIROperand cond = visit_Expr(aw, s->condition);

        emit_single(aw, WIR_IF_BEGIN, cond);
        visit_Stmt(aw, s->then_branch);

        if (s->else_branch) {
            emit_simple(aw, WIR_ELSE);
            visit_Stmt(aw, s->else_branch);
        }

        emit_simple(aw, WIR_IF_END);

        return;
    }
    case NODE_StmtLoop: {
        StmtLoop *s = (StmtLoop *)stmt;

        if (s->count) {
            WIROperand count = visit_Expr(aw, s->count);
            emit_single(aw, WIR_LOOP_BEGIN_N, count);
        } else {
            emit_simple(aw, WIR_LOOP_BEGIN);
        }
        visit_Stmt(aw, s->body);

        emit_simple(aw, WIR_LOOP_END);
        return;
    }
    case NODE_StmtFor: {
        StmtFor *s = (StmtFor *)stmt;
        visit_Expr(aw, s->left_bound);
        visit_Expr(aw, s->right_bound);
        visit_Stmt(aw, s->body);
        return;
    }
    case NODE_StmtContinue:
        emit_simple(aw, WIR_CONTINUE);
        return;
    case NODE_StmtBreak: 
        emit_simple(aw, WIR_BREAK);
        return;
    case NODE_StmtCmd: {
        StmtCmd *s = (StmtCmd *)stmt;

        VEC_WIROperand ops = VEC_EMPTY;

        {
            assert(s->id->kind == NODE_ExprIntLit);
            ExprIntLit *cmd_id = (ExprIntLit *)s->id;
            VEC_PUSH(ops, WIR_IMM_I(cmd_id->value), aw->arena);

            // TODO: Currently, let all relative indent values be 0.
            VEC_PUSH(ops, WIR_IMM_I(0), aw->arena);
        }

        for (size_t i = 0; i < s->int_operands.count; i++)
            VEC_PUSH(ops, visit_Expr(aw, s->int_operands.at[i]), aw->arena);
        for (size_t i = 0; i < s->str_operands.count; i++)
            VEC_PUSH(ops, visit_Expr(aw, s->str_operands.at[i]), aw->arena);

        WIRInst inst = (WIRInst) {
            .op = WIR_CMD,
            .operands = ops
        };

        emit_to_current_cev(aw, inst);
        return;
    }
    case NODE_StmtExpr: {
        StmtExpr *s = (StmtExpr *)stmt;
        visit_Expr(aw, s->expr);
        return;
    }
    case NODE_StmtDBDecl: {
        StmtDBDecl *s = (StmtDBDecl *)stmt;
        VEC_PUSH(aw->wir->cdb_types,
            ((WIRDB){
                .debug_name = s->name,
                .fields = VEC_EMPTY
            }), aw->arena);
        for (size_t i = 0; i < s->fields.count; i++)
            visit_db_field_decl(aw, (Stmt *)s->fields.at[i]);
        return;
    }
    }
}

void ast2wir_pass(VEC_Module *modules, Arena *arena) {
    for (size_t i = 0; i < modules->count; i++) {
        Ast2Wir aw = (Ast2Wir){
            .arena = arena,
            .tmp = { 0 },
            .global = { 0 },
            .wir = arena_alloc_assert(arena, sizeof(WIR)),
            .local_frames = VEC_EMPTY,
        };

        Module *mod = &modules->at[modules->count - i - 1];
        for (size_t j = 0; j < mod->ast->stmts.count; j++) {
            visit_Stmt(&aw, mod->ast->stmts.at[j]);
            assert(aw.local_frames.count == 0);
        }

        mod->wir = aw.wir;
    }
}