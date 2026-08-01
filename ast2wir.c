#include "ast2wir.h"
#include "parser.h"
#include "wir.h"

#define ALLOC_WIR(var, type_, ...) \
    do { \
        (var) = arena_alloc_assert(aw->arena, sizeof(type_)); \
        *var = __VA_ARGS__; \
        (var)->base.kind = INST_##type_; \
    } while (0)

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

static void emit_to_current_cev(Ast2Wir *aw, WIRInst *inst) {
    assert(aw->wir->g_cevs.count > 0);

    WIRCev *last = &aw->wir->g_cevs.at[aw->wir->g_cevs.count - 1];
    VEC_PUSH(last->insts, inst, aw->arena);
}

static void emit_binop(Ast2Wir *aw, WIROperand dest, WIROperand a, WIROperand b, int op) {
    WIRInst_Binop *inst;
    ALLOC_WIR(inst, WIRInst_Binop,
        (WIRInst_Binop){ .dest = dest, .op = op, .a = a, .b = b });
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
    return (WIROperand){
        .kind = OPKIND_TEMP,
        .as.local = { 
            .type = LOCAL_INT, 
            .offset = aw->tmp.int_top++ 
    }};
}

static WIROperand tmp_str(Ast2Wir *aw) {
    return (WIROperand){
        .kind = OPKIND_TEMP,
        .as.local = { 
            .type = LOCAL_STR, 
            .offset = aw->tmp.str_top++ 
    }};
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

static WIROperand visit_Expr(Ast2Wir *aw, Expr *expr);

static WIROperand _visit_Expr(Ast2Wir *aw, Expr *expr) {
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
                .kind = OPKIND_LOCAL,
                .as.local = {
                    .type = e->sym->type.basetype == TYPE_STR ?
                        LOCAL_STR : LOCAL_INT,
                    .offset = e->sym->offset 
            }};
        } else {
            int global_type = 0;
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
                global_type = GLOBAL_INT;
                break;
            case TYPE_STR:
                global_type = GLOBAL_STR;
                break;
            case TYPE_FUNC:
                global_type = GLOBAL_CEV;
                break;
            case TYPE_ARRAY: UNIMPLEMENTED;
            }

            return (WIROperand){
                .kind = OPKIND_GLOBAL,
                .as.global = {
                    .type = global_type,
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

        WIROperand dest = tmp_int(aw);
        WIROperand callee = visit_Expr(aw, e->callee);
        VEC_WIROperand args = VEC_EMPTY;
        for (size_t i = 0; i < e->args.count; i++) {
            VEC_PUSH(args,
                visit_Expr(aw, e->args.at[i]), aw->arena);
        }

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
    }

    UNREACHABLE;
    return (WIROperand){ 0 };
}

static WIROperand visit_Expr(Ast2Wir *aw, Expr *expr) {
    TwoStack top = aw->tmp;
    WIROperand operand = _visit_Expr(aw, expr);
    aw->tmp = top;

    // The temporary result of a calculation needs a slot to hold it.
    if (operand.kind == OPKIND_TEMP) {
        switch (operand.as.local.type) {
        case LOCAL_INT: aw->tmp.int_top++; break;
        case LOCAL_STR: aw->tmp.str_top++; break;
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

        switch (s->sym->type.basetype) {
        case TYPE_STR: {
            s->sym->offset = new_local_str(aw);

            WIROperand init = { 0 };
            if (s->initializer) {
                init = visit_Expr(aw, s->initializer);
                emit_str(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL,
                        .as.local = {
                            .type = LOCAL_STR,
                            .offset = s->sym->offset
                    }},
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
                        .kind = OPKIND_LOCAL,
                        .as.local = {
                            .type = LOCAL_INT,
                            .offset = s->sym->offset
                    }},
                    init, WIR_IMM_I(0), WIR_ADD);
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
            emit_simple(aw, INST_WIRInst_ReturnVoid);
        }

        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        WIROperand cond = visit_Expr(aw, s->condition);

        WIRInst_IfBegin *inst;
        ALLOC_WIR(inst, WIRInst_IfBegin,
            (WIRInst_IfBegin){ .cond = cond });
        emit_to_current_cev(aw, (WIRInst *)inst);

        visit_Stmt(aw, s->then_branch);

        if (s->else_branch) {
            emit_simple(aw, INST_WIRInst_Else);
            visit_Stmt(aw, s->else_branch);
        }

        emit_simple(aw, INST_WIRInst_IfEnd);

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
            emit_simple(aw, INST_WIRInst_LoopBegin);
        }
        visit_Stmt(aw, s->body);

        emit_simple(aw, INST_WIRInst_LoopEnd);
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
        emit_simple(aw, INST_WIRInst_Continue);
        return;
    case NODE_StmtBreak: 
        emit_simple(aw, INST_WIRInst_Break);
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
    case NODE_StmtExpr: {
        StmtExpr *s = (StmtExpr *)stmt;
        visit_Expr(aw, s->expr);
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
        wir_init(aw.wir);

        Module *mod = &modules->at[modules->count - i - 1];
        for (size_t j = 0; j < mod->ast->stmts.count; j++) {
            visit_Stmt(&aw, mod->ast->stmts.at[j]);
            assert(aw.local_frames.count == 0);
        }

        mod->wir = aw.wir;
    }
}