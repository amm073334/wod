#include "ast2wir.h"
#include "parser.h"

#define ALLOC_WIR(var, type_, ...) \
    do { \
        (var) = arena_alloc_assert(aw->arena, sizeof(type_)); \
        *var = __VA_ARGS__; \
        (var)->base.kind = _##type_; \
    } while (0)

#define WOP(...) (RetVal){ .rv = RV_WOP, .as.wop = (__VA_ARGS__) }

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

    Module *current_module;

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

typedef struct RetVal RetVal;
struct RetVal {
    enum {
        RV_WOP,
        RV_DBDATA,
        RV_DBFIELD,
        RV_ARRAYLIT
    } rv;
    union {
        WIROperand wop;
        struct {
            WIROperand type_id;
            WIROperand data_id;
        } dbdata;
        struct {
            DBKind kind;
            WIROperand type_id;
            WIROperand data_id;
            int32_t field_id;
        } dbfield;
        VEC_WIROperand array_lit;
    } as;
};

static void emit_load(Ast2Wir *aw, WIROperand dst, WIRAssign assign, RetVal field, int32_t offset) {
    WIRInst_DBLoad *inst;
    ALLOC_WIR(inst, WIRInst_DBLoad, (WIRInst_DBLoad){
        .dst = dst,
        .assign = assign,
        .db_kind = field.as.dbfield.type_id.kind 
            == OPKIND_GLOBAL_UDBTYPE ? DB_UDB : DB_CDB, 
        .db_type = field.as.dbfield.type_id,
        .db_data = field.as.dbfield.data_id,
        .db_field = WIR_IMM_I(field.as.dbfield.field_id + offset),
    });
    emit_to_current_cev(aw, (WIRInst *)inst);
}

static void emit_store(Ast2Wir *aw, RetVal field, WIRAssign assign, WIROperand src, int32_t offset) {
    WIRInst_DBStore *inst;
    ALLOC_WIR(inst, WIRInst_DBStore, (WIRInst_DBStore){
        .src = src,
        .assign = assign,
        .db_kind = field.as.dbfield.type_id.kind 
            == OPKIND_GLOBAL_UDBTYPE ? DB_UDB : DB_CDB, 
        .db_type = field.as.dbfield.type_id,
        .db_data = field.as.dbfield.data_id,
        .db_field = WIR_IMM_I(field.as.dbfield.field_id + offset),
    });
    emit_to_current_cev(aw, (WIRInst *)inst);
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

static size_t new_local_int(Ast2Wir *aw, size_t n) {
    assert(aw->local_frames.count > 0);
    WIRInst_PushIntN *inst;
    ALLOC_WIR(inst, WIRInst_PushIntN, (WIRInst_PushIntN){ .n = n });
    emit_to_current_cev(aw, (WIRInst *)inst);
    Frame *top = &aw->local_frames.at[aw->local_frames.count - 1];
    size_t old = top->int_top;
    top->int_top += n;
    return old;
}

static size_t new_local_str(Ast2Wir *aw, size_t n) {
    assert(aw->local_frames.count > 0);
    WIRInst_PushStrN *inst;
    ALLOC_WIR(inst, WIRInst_PushStrN, (WIRInst_PushStrN){ .n = n });
    emit_to_current_cev(aw, (WIRInst *)inst);
    Frame *top = &aw->local_frames.at[aw->local_frames.count - 1];
    size_t old = top->str_top;
    top->str_top += n;
    return old;
}

static void visit_db_field_decl(Ast2Wir *aw, StmtVarDecl *s, WIRDB *db) {
    assert(!s->is_const);

    int type = 0;
    if (s->sym->type.basetype == TYPE_ARRAY) {
        switch (s->sym->type.array_of->basetype) {
            case TYPE_STR: type = WIRFIELD_STR; break;
            case TYPE_INT:
            case TYPE_BOOL:
                type = WIRFIELD_INT;
                break;
            default: UNREACHABLE;
        }
    } else {
        switch (s->sym->type.basetype) {
            case TYPE_STR: type = WIRFIELD_STR; break;
            case TYPE_INT:
            case TYPE_BOOL:
                type = WIRFIELD_INT;
                break;
            default: UNREACHABLE;
        }
    }

    WIRField field = {
        .name = s->name,
        .type = type,
        .has_initializer = false
    };

    if (s->initializer) {
        assert(s->initializer->type.is_constexpr);
        field.has_initializer = true;

        switch (s->initializer->kind) {
        case NODE_ExprIntLit: {
            ExprIntLit *lit = (ExprIntLit *)s->initializer;
            field.initializer = (WIROperand){
                .kind = OPKIND_IMM_INT,
                .as.imm_int = lit->value
            };
            break;
        }
        case NODE_ExprBoolLit: {
            ExprBoolLit *lit = (ExprBoolLit *)s->initializer;
            field.initializer = (WIROperand){
                .kind = OPKIND_IMM_INT,
                .as.imm_int = lit->value
            };
            break;
        }
        case NODE_ExprStrLit:
        default: UNREACHABLE;
        }
    }

    VEC_PUSH(db->fields, field, aw->arena);
    
    if (s->sym->type.basetype == TYPE_ARRAY) {
        field.name = SV("");
        for (int32_t i = 0; i < s->sym->type.array_len - 1; i++) {
            VEC_PUSH(db->fields, field, aw->arena);
        }
    }
}

static void visit_db_data(Ast2Wir *aw, ExprDBDataElem *e, WIRDB *db, size_t data_index) {
    if (e->no_body)
        return;

    WIRData data = { .name = e->name, .values = VEC_EMPTY };
    for (size_t i = 0; i < db->fields.count; i++) {

        // Array elements are named with the empty string if they aren't the first element.
        if (sv_equals(db->fields.at[i].name, SV(""))) continue;
        
        bool found = false;
        for (size_t j = 0; j < e->fields.count; j++) {
            if (!sv_equals(db->fields.at[i].name, e->fields.at[j]->name)) continue;

            found = true;

            Expr *value = e->fields.at[j]->value;
            assert(value->type.is_constexpr);

            switch(value->kind) {
                case NODE_ExprIntLit: {
                    WIROperand wop = (WIROperand){
                        .kind = OPKIND_IMM_INT,
                        .as.imm_int = ((ExprIntLit *)value)->value
                    };
                    VEC_PUSH(data.values, wop, aw->arena);
                    break;
                }
                case NODE_ExprBoolLit: {
                    WIROperand wop = (WIROperand){
                        .kind = OPKIND_IMM_INT,
                        .as.imm_int = ((ExprBoolLit *)value)->value
                    };
                    VEC_PUSH(data.values, wop, aw->arena);
                    break;
                }
                case NODE_ExprStrLit: {
                    WIROperand wop = (WIROperand){
                        .kind = OPKIND_IMM_STR,
                        .as.imm_str = ((ExprStrLit *)value)->value
                    };
                    VEC_PUSH(data.values, wop, aw->arena);
                    break;
                }
                case NODE_ExprArrayLit: {
                    ExprArrayLit *elem = (ExprArrayLit *)value;
                    for (size_t k = 0; k < elem->values.count; k++) {
                        Expr *node = elem->values.at[k];
                        switch (node->kind) {
                            case NODE_ExprIntLit: {
                                WIROperand wop = (WIROperand){
                                    .kind = OPKIND_IMM_INT,
                                    .as.imm_int = ((ExprIntLit *)node)->value
                                };
                                VEC_PUSH(data.values, wop, aw->arena);
                                break;
                            }
                            case NODE_ExprBoolLit: {
                                WIROperand wop = (WIROperand){
                                    .kind = OPKIND_IMM_INT,
                                    .as.imm_int = ((ExprBoolLit *)node)->value
                                };
                                VEC_PUSH(data.values, wop, aw->arena);
                                break;
                            }
                            case NODE_ExprStrLit: {
                                WIROperand wop = (WIROperand){
                                    .kind = OPKIND_IMM_STR,
                                    .as.imm_str = ((ExprStrLit *)node)->value
                                };
                                VEC_PUSH(data.values, wop, aw->arena);
                                break;
                            }
                            default: UNREACHABLE;
                        }
                    }
                    break;
                }
                default: UNREACHABLE;
            }
            break;
        }

        if (!found) {
            assert(db->fields.at[i].has_initializer);
            VEC_PUSH(data.values, db->fields.at[i].initializer, aw->arena);
        }
    }

    db->data.at[data_index] = data;
}

static WIROperand *heapalloc(Arena *arena, WIROperand wop) {
    WIROperand *out = arena_alloc_assert(arena, sizeof(WIROperand));
    *out = wop;
    return out;
}

static int type_to_global_kind(WodType *wt) {
    switch (wt->basetype) {
    case TYPE_NONE:
    case TYPE_ERROR:
    case TYPE_VOID:
    case TYPE_MODULE:
    case TYPE_CEVTYPE:
        UNREACHABLE;
        return 0;
    case TYPE_INT:
    case TYPE_BOOL:
    case TYPE_PTR:
    case TYPE_DBDATA:
        return OPKIND_GLOBAL_INT;
    case TYPE_STR:
        return OPKIND_GLOBAL_STR;
    case TYPE_FUNC:
        return OPKIND_GLOBAL_CEV;
    case TYPE_DBTYPE:
        if (wt->db_kind == DB_UDB)
            return OPKIND_GLOBAL_UDBTYPE;
        else 
            return OPKIND_GLOBAL_CDBTYPE;
        break;
    case TYPE_ARRAY:
        return type_to_global_kind(wt->array_of);
    }
    return 0;
}

static RetVal visit_Expr(Ast2Wir *aw, Expr *expr);

static WIROperand into_temp(Ast2Wir *aw, RetVal rv, Expr *expr) {
    if (rv.rv == RV_WOP) return rv.as.wop;
    if (rv.rv == RV_DBDATA) return rv.as.dbdata.data_id;

    assert(rv.rv == RV_DBFIELD);
    
    WIROperand temp = { 0 };
    switch (expr->type.basetype) {
        case TYPE_INT:
        case TYPE_BOOL:
            temp = tmp_int(aw);
            break;
        case TYPE_STR:
            temp = tmp_str(aw);
            break;
        case TYPE_ARRAY:
            if (expr->type.array_of->basetype == TYPE_STR)
                temp = tmp_int(aw);
            else
                temp = tmp_str(aw);
            break;
        default: UNREACHABLE;
    }

    emit_load(aw, temp, WIR_ASSIGN_EQ, rv, 0);

    return temp;
}

// A weird function that mostly works the same as `visit_Expr()`, except
// it ensures that the result is a single `WIROperand`. In particular:
// - If the return value was a DB field, the field is copied to a temporary first.
// - If the return value was DB data, the data ID is returned.
//   This is to facilitate assigning to dbdata-type variables.
// - The return value should never be an array. This causes an error. 
static WIROperand eval(Ast2Wir *aw, Expr *expr) {
    RetVal rv = visit_Expr(aw, expr);

    return into_temp(aw, rv, expr);
}

static RetVal visit_Expr(Ast2Wir *aw, Expr *expr) {
    switch (expr->kind) {
    case NODE_ExprVar: {
        ExprVar *e = (ExprVar *)expr;

        if (expr->type.is_constexpr) {
            switch (expr->type.basetype) {
            case TYPE_INT: return WOP(WIR_IMM_I(e->sym->const_i));
            case TYPE_STR: return WOP(WIR_IMM_S(e->sym->const_s));
            case TYPE_BOOL: return WOP(WIR_IMM_I(e->sym->const_b));
            default: UNREACHABLE;
            }
        }

        if (sv_is_null(e->sym->top_level_path)) {
            if (e->sym->type.basetype == TYPE_ARRAY) {
                return WOP((WIROperand){
                    .kind = e->sym->type.array_of->basetype == TYPE_STR ?
                        OPKIND_LOCAL_STR : OPKIND_LOCAL_INT,
                    .as.offset = e->sym->local_offset 
                });
            }

            return WOP((WIROperand){
                .kind = e->sym->type.basetype == TYPE_STR ?
                    OPKIND_LOCAL_STR : OPKIND_LOCAL_INT,
                .as.offset = e->sym->local_offset 
            });
        } else {
            int kind = type_to_global_kind(&e->sym->type);
            return WOP((WIROperand){
                .kind = kind,
                .as.global = {
                    .path = e->sym->top_level_path,
                    .name = e->sym->name
            }});
        }
    }
    case NODE_ExprArray: {
        ExprArray *e = (ExprArray *)expr;
        
        WIROperand index = eval(aw, e->index);
        RetVal left = visit_Expr(aw, e->left);
        if (e->left->type.basetype == TYPE_ARRAY)  {

            // Arrays can only be indexed by integer-type constant expressions.
            assert(index.kind == OPKIND_IMM_INT);

            // It shouldn't be possible to index into DB data.
            assert(left.rv != RV_DBDATA);

            if (left.rv == RV_DBFIELD) {
                RetVal out = left;
                out.as.dbfield.field_id += index.as.imm_int;
                return out;
            }

            assert(left.rv == RV_WOP);
            if (op_is_local(left.as.wop)) {
                RetVal out = left;
                out.as.wop.as.offset += index.as.imm_int;
                return out;
            }

            // Global operand.
            RetVal out = left;
            out.as.wop.as.global.id += index.as.imm_int;
            return out;
        }
        
        assert(e->left->type.basetype == TYPE_DBTYPE);
        assert(left.rv == RV_WOP);
        
        return (RetVal){
            .rv = RV_DBDATA,
            .as.dbdata = {
                .type_id = left.as.wop,
                .data_id = index
            }
        };
    }
    case NODE_ExprAccess: {
        ExprAccess *e = (ExprAccess *)expr;

        RetVal left = visit_Expr(aw, e->left);

        if (left.rv == RV_DBDATA) {
            return (RetVal){
                .rv = RV_DBFIELD,
                .as.dbfield = {
                    .type_id = left.as.dbdata.type_id,
                    .data_id = left.as.dbdata.data_id,
                    .field_id = (int32_t)e->sym->local_offset
                }
            };
        }
        
        assert(left.rv == RV_WOP);
        assert(left.as.wop.kind == OPKIND_GLOBAL_UDBTYPE);
        
        // Find index.
        assert(e->left->type.basetype == TYPE_DBTYPE);
        Symbol *data = env_find(e->left->type.db_named_data, e->name.text);
        assert(data);

        return (RetVal){
            .rv = RV_DBDATA,
            .as.dbdata = {
                .type_id = left.as.wop,
                .data_id = WIR_IMM_I((int32_t)data->local_offset)
            }
        };
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
            
            WIROperand left = eval(aw, e->left);
            emit_if_begin(aw, left);
            
            WIROperand right = eval(aw, e->right);
            emit_if_begin(aw, right);

            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            emit_simple(aw, _WIRInst_IfEnd);
            emit_simple(aw, _WIRInst_IfEnd);

            return WOP(dest);
        } else if (e->op.type == TOK_PIPE_PIPE) {
            // By De Morgan's laws, a || b can be expressed as !(!a && !b).
            // Doing things this way saves us from having to either duplicate code
            // or use a label/goto. (Whether or not that is good performance-wise
            // needs investigation.)

            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            WIROperand left = eval(aw, e->left);

            WIROperand tmp_left = tmp_int(aw);
            emit_binop(aw, tmp_left, WIR_ASSIGN_EQ, left, WIR_IMM_I(1), WIR_BINOP_XOR);
            emit_if_begin(aw, tmp_left);

            WIROperand right = eval(aw, e->right);

            WIROperand tmp_right = tmp_int(aw);
            emit_binop(aw, tmp_right, WIR_ASSIGN_EQ, right, WIR_IMM_I(1), WIR_BINOP_XOR);
            emit_if_begin(aw, tmp_right);

            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(0), WIR_IMM_I(0), WIR_BINOP_ADD);
            
            emit_simple(aw, _WIRInst_IfEnd);
            emit_simple(aw, _WIRInst_IfEnd);

            return WOP(dest);
        }

        WIROperand left = eval(aw, e->left);
        WIROperand right = eval(aw, e->right);

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

        return WOP(dest);
    }
    case NODE_ExprUnary: {
        ExprUnary *e = (ExprUnary *)expr;

        WIROperand right = eval(aw, e->right);

        switch (e->op.type) {
        case TOK_MINUS: {
            WIROperand dest = tmp_int(aw);
            emit_binop(aw, dest, WIR_ASSIGN_EQ, WIR_IMM_I(0), right, WIR_BINOP_SUB);
            return WOP(dest);
        }
        case TOK_BANG: {
            // Assumes that booleans are either zero or one.
            WIROperand dest = tmp_int(aw);
            emit_binop(aw, dest, WIR_ASSIGN_EQ, right, WIR_IMM_I(1), WIR_BINOP_XOR);
            return WOP(dest);
        }
        case TOK_AMP: {
            assert(op_is_local(right) || op_is_global(right));
            return WOP(right);
        }
        default: UNREACHABLE;
        }

        return (RetVal){ 0 };
    }
    case NODE_ExprCall: {
        ExprCall *e = (ExprCall *)expr;

        WIROperand callee = eval(aw, e->callee);
        
        VEC_WIROperand args = VEC_EMPTY;
        for (size_t i = 0; i < e->args.count; i++)
            VEC_PUSH(args, eval(aw, e->args.at[i]), aw->arena);

        WIROperand dest;
        if (e->base.type.basetype == TYPE_STR)
            dest = tmp_str(aw);
        else
            dest = tmp_int(aw);

        WIRInst_Call *inst;
        ALLOC_WIR(inst, WIRInst_Call,
            (WIRInst_Call){.dest = dest, .cev = callee, .args = args});
        emit_to_current_cev(aw, (WIRInst *)inst);

        return WOP(dest);
    }

    case NODE_ExprIntLit: {
        ExprIntLit *e = (ExprIntLit *)expr;
        return WOP(WIR_IMM_I(e->value));
    }
    case NODE_ExprStrLit: {
        ExprStrLit *e = (ExprStrLit *)expr;
        return WOP(WIR_IMM_S(e->value));
    }
    case NODE_ExprBoolLit: {
        ExprBoolLit *e = (ExprBoolLit *)expr;
        return WOP(WIR_IMM_I(e->value));
    }
    case NODE_ExprArrayLit: {
        ExprArrayLit *e = (ExprArrayLit *)expr;

        VEC_WIROperand ops = VEC_EMPTY;
        for (size_t i = 0; i < e->values.count; i++) {
            VEC_PUSH(ops, eval(aw, e->values.at[i]), aw->arena);
        }

        return (RetVal){
            .rv = RV_ARRAYLIT,
            .as.array_lit = ops
        };
    }
    case NODE_ExprInterp: {
        VEC_WIROperand results = VEC_EMPTY;

        Expr *p = expr;
        while (p->kind == NODE_ExprInterp) {
            ExprInterp *e = (ExprInterp *)p;
            VEC_PUSH(results, WIR_IMM_S(e->opening), aw->arena);

            RetVal rv = visit_Expr(aw, e->expr);
            if (rv.rv == RV_ARRAYLIT) {
                VEC_PUSH(results, WIR_IMM_S(SV("[")), aw->arena);
                for (size_t i = 0; i < rv.as.array_lit.count; i++) {
                    VEC_PUSH(results, rv.as.array_lit.at[i], aw->arena);
                    if (i < (size_t)e->expr->type.array_len - 1)
                        VEC_PUSH(results, WIR_IMM_S(SV(", ")), aw->arena);
                }
                VEC_PUSH(results, WIR_IMM_S(SV("]")), aw->arena);
            } else {
                if (e->expr->type.basetype == TYPE_ARRAY) {
                    VEC_PUSH(results, WIR_IMM_S(SV("[")), aw->arena);

                    if (rv.rv == RV_WOP) {
                        WIROperand elem = into_temp(aw, rv, e->expr);
                        for (int32_t i = 0; i < e->expr->type.array_len; i++) {
                            VEC_PUSH(results, elem, aw->arena);
                            if (i < e->expr->type.array_len - 1)
                                VEC_PUSH(results, WIR_IMM_S(SV(", ")), aw->arena);
        
                            if (op_is_local(elem))
                                elem.as.offset++;
                            else
                                elem.as.global.id++;
                        }
                    } else if (rv.rv == RV_DBFIELD) {
                        for (int32_t i = 0; i < e->expr->type.array_len; i++) {
                            WIROperand temp = { 0 };
                            if (e->expr->type.array_of->basetype == TYPE_STR)
                                temp = tmp_str(aw);
                            else
                                temp = tmp_int(aw);
                            emit_load(aw, temp, WIR_ASSIGN_EQ, rv, i);
                            VEC_PUSH(results, temp, aw->arena);
                            if (i < e->expr->type.array_len - 1)
                                VEC_PUSH(results, WIR_IMM_S(SV(", ")), aw->arena);
                        }
                    }
                    VEC_PUSH(results, WIR_IMM_S(SV("]")), aw->arena);
                } else {
                    WIROperand wop = into_temp(aw, rv, e->expr);
                    VEC_PUSH(results, wop, aw->arena);
                }
            }
            p = e->next;
        }
        assert(p->kind == NODE_ExprStrLit);

        VEC_PUSH(results, eval(aw, p), aw->arena);

        return WOP((WIROperand){
            .kind = OPKIND_INTERP,
            .as.interp = results
        });
    }
    case NODE_ExprStructLitField:
    case NODE_ExprDBDataElem:
    case NODE_ExprPrimType:
        UNREACHABLE;
    }

    UNREACHABLE;
    return (RetVal){ 0 };
}

static void visit_Stmt(Ast2Wir *aw, Stmt *stmt) {
    switch (stmt->kind) {
    case NODE_StmtAssign: {
        StmtAssign *s = (StmtAssign *)stmt;

        RetVal right = visit_Expr(aw, s->right);
        RetVal left = visit_Expr(aw, s->left);

        WIRAssign assign = WIR_ASSIGN_EQ;
        switch (s->assign_type.type) {
            case TOK_EQUAL:         assign = WIR_ASSIGN_EQ; break;
            case TOK_PLUS_EQUAL:    assign = WIR_ASSIGN_ADD; break;
            case TOK_MINUS_EQUAL:   assign = WIR_ASSIGN_SUB; break;
            case TOK_STAR_EQUAL:    assign = WIR_ASSIGN_MUL; break;
            case TOK_SLASH_EQUAL:   assign = WIR_ASSIGN_DIV; break;
            case TOK_PERCENT_EQUAL: assign = WIR_ASSIGN_MOD; break;
            case TOK_AMP_EQUAL:     
            case TOK_PIPE_EQUAL:
                break;
            default: UNREACHABLE;
        }

        // For arrays, copy all elements.
        if (s->left->type.basetype == TYPE_ARRAY) {
            assert(assign == WIR_ASSIGN_EQ);

            if (right.rv == RV_ARRAYLIT) {
                assert((size_t)s->left->type.array_len == right.as.array_lit.count);

                if (left.rv == RV_WOP) {
                    WIROperand elem_left = left.as.wop;

                    for (int32_t i = 0; i < s->left->type.array_len; i++) {
                        if (op_is_string(right.as.array_lit.at[i]))
                            emit_str(aw, elem_left, right.as.array_lit.at[i]);
                        else    
                            emit_binop(aw, elem_left, assign, right.as.array_lit.at[i], WIR_IMM_I(0), WIR_BINOP_ADD);
                        
                        // Increment elements.
                        if (op_is_local(elem_left))
                            elem_left.as.offset++;
                        else
                            elem_left.as.global.id++;
        
                    }
                    return;
                } else if (left.rv == RV_DBFIELD) {
                    for (int32_t i = 0; i < s->left->type.array_len; i++) {
                        emit_store(aw, left, assign, right.as.array_lit.at[i], i);
                    }
                    return;
                }

                UNREACHABLE;
            }            

            if (left.rv == RV_WOP && right.rv == RV_WOP) {
                WIROperand elem_left = left.as.wop;
                WIROperand elem_right = right.as.wop;
    
                for (int32_t i = 0; i < s->left->type.array_len; i++) {
                    if (op_is_string(right.as.wop))
                        emit_str(aw, elem_left, elem_right);
                    else    
                        emit_binop(aw, elem_left, assign, elem_right, WIR_IMM_I(0), WIR_BINOP_ADD);
                    
                    // Increment elements.
                    if (op_is_local(elem_left))
                        elem_left.as.offset++;
                    else
                        elem_left.as.global.id++;
    
                    if (op_is_local(elem_right))
                        elem_right.as.offset++;
                    else
                        elem_right.as.global.id++;
    
                }
                return;
            } else if (left.rv == RV_WOP) {
                assert(right.rv == RV_DBFIELD);
                WIROperand elem_left = left.as.wop;
    
                for (int32_t i = 0; i < s->left->type.array_len; i++) {
                    emit_load(aw, elem_left, assign, right, i);

                    // Increment elements.
                    if (op_is_local(elem_left))
                        elem_left.as.offset++;
                    else
                        elem_left.as.global.id++;
                }
                return;
            } else if (right.rv == RV_WOP) {
                assert(left.rv == RV_DBFIELD);
                WIROperand elem_right = right.as.wop;
    
                for (int32_t i = 0; i < s->left->type.array_len; i++) {
                    emit_store(aw, left, assign, elem_right, i);

                    // Increment elements.
                    if (op_is_local(elem_right))
                        elem_right.as.offset++;
                    else
                        elem_right.as.global.id++;
                }
                return;
            } else {
                assert(left.rv == RV_DBFIELD && right.rv == RV_DBFIELD);

                // Only one temporary is needed to copy all the elements.
                WIROperand temp = { 0 };
                switch (s->left->type.array_of->basetype) {
                    case TYPE_INT:
                    case TYPE_BOOL:
                        temp = tmp_int(aw);
                        break;
                    case TYPE_STR:
                        temp = tmp_str(aw);
                        break;
                    default: UNREACHABLE;
                }

                for (int32_t i = 0; i < s->left->type.array_len; i++) {
                    emit_load(aw, temp, assign, right, i);
                    emit_store(aw, left, assign, temp, i);
                }
                return;
            }
        }

        if (left.rv == RV_WOP && right.rv == RV_WOP) {
            if (op_is_string(right.as.wop)) {
                assert(op_is_string(left.as.wop));
                assert(assign == WIR_ASSIGN_EQ);
                emit_str(aw, left.as.wop, right.as.wop);
                return;
            }

            if (s->assign_type.type == TOK_AMP_EQUAL) {
                emit_binop(aw, left.as.wop, WIR_ASSIGN_EQ, left.as.wop, right.as.wop, WIR_BINOP_AND);
            } else if (s->assign_type.type == TOK_PIPE_EQUAL) {
                emit_binop(aw, left.as.wop, WIR_ASSIGN_EQ, left.as.wop, right.as.wop, WIR_BINOP_OR);
            } else {
                emit_binop(aw, left.as.wop, assign, right.as.wop, WIR_IMM_I(0), WIR_BINOP_ADD);
            }

            return;
        } else if (left.rv == RV_WOP) {
            if (s->assign_type.type == TOK_AMP_EQUAL
                || s->assign_type.type == TOK_PIPE_EQUAL) {
            
                WIROperand temp = tmp_int(aw);
    
                emit_load(aw, temp, WIR_ASSIGN_EQ, right, 0);
                        
                if (s->assign_type.type == TOK_AMP_EQUAL) {
                    emit_binop(aw, left.as.wop, WIR_ASSIGN_EQ, left.as.wop, temp, WIR_BINOP_AND);
                } else if (s->assign_type.type == TOK_PIPE_EQUAL) {
                    emit_binop(aw, left.as.wop, WIR_ASSIGN_EQ, left.as.wop, temp, WIR_BINOP_OR);
                }

                return;
            }

            emit_load(aw, left.as.wop, assign, right, 0);

            return;
        } else if (right.rv == RV_WOP) {
            if (s->assign_type.type == TOK_AMP_EQUAL
                || s->assign_type.type == TOK_PIPE_EQUAL) {
                    
                WIROperand temp = tmp_int(aw);

                emit_load(aw, temp, WIR_ASSIGN_EQ, left, 0);

                if (s->assign_type.type == TOK_AMP_EQUAL) {
                    emit_binop(aw, temp, WIR_ASSIGN_EQ, temp, right.as.wop, WIR_BINOP_AND);
                } else if (s->assign_type.type == TOK_PIPE_EQUAL) {
                    emit_binop(aw, temp, WIR_ASSIGN_EQ, temp, right.as.wop, WIR_BINOP_OR);
                }

                right = WOP(temp);
            }

            emit_store(aw, left, assign, right.as.wop, 0);
            return;
        }

        assert(left.rv == RV_DBFIELD && right.rv == RV_DBFIELD);
        
        WIROperand r_temp = tmp_int(aw);
        emit_load(aw, r_temp, WIR_ASSIGN_EQ, right, 0);
                
        if (s->assign_type.type == TOK_AMP_EQUAL
            || s->assign_type.type == TOK_PIPE_EQUAL) {
                
            WIROperand l_temp = tmp_int(aw);
            emit_load(aw, l_temp, WIR_ASSIGN_EQ, left, 0);

            if (s->assign_type.type == TOK_AMP_EQUAL) {
                emit_binop(aw, l_temp, WIR_ASSIGN_EQ, l_temp, r_temp, WIR_BINOP_AND);
            } else if (s->assign_type.type == TOK_PIPE_EQUAL) {
                emit_binop(aw, l_temp, WIR_ASSIGN_EQ, l_temp, r_temp, WIR_BINOP_OR);
            }

            r_temp = l_temp;
        }

        emit_store(aw, left, assign, r_temp, 0);

        return;
    }
    case NODE_StmtVarDecl: {
        StmtVarDecl *s = (StmtVarDecl *)stmt;

        if (s->sym->type.basetype == TYPE_ARRAY) {
            switch (s->sym->type.array_of->basetype) {
            case TYPE_STR: {
                if (!sv_is_null(s->sym->top_level_path)) {
                    assert(!s->initializer);
                    for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                        VEC_PUSH(aw->wir->g_strs, ((Qualifier){
                            .path = s->sym->top_level_path,
                            .name = s->name,
                            .id = i
                        }), aw->arena);
                    }
                    break;
                }

                s->sym->local_offset = new_local_str(aw, s->sym->type.array_len);
                if (s->initializer) {
                    RetVal init = visit_Expr(aw, s->initializer);

                    if (init.rv == RV_WOP) {
                        WIROperand elem = init.as.wop;
            
                        for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                            emit_str(aw,
                                (WIROperand){
                                    .kind = OPKIND_LOCAL_STR,
                                    .as.offset = s->sym->local_offset + i
                                }, elem);
            
                            if (op_is_local(elem))
                                elem.as.offset++;
                            else
                                elem.as.global.id++;
            
                        }
                    } else if (init.rv == RV_DBFIELD) {
                        for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                            emit_load(aw,
                                (WIROperand){
                                    .kind = OPKIND_LOCAL_STR,
                                    .as.offset = s->sym->local_offset + i
                                }, WIR_ASSIGN_EQ, init, i);
                        }
                    } else if (init.rv == RV_ARRAYLIT) {
                        assert((size_t)s->sym->type.array_len == init.as.array_lit.count);
                        for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                            emit_str(aw,
                                (WIROperand){
                                    .kind = OPKIND_LOCAL_STR,
                                    .as.offset = s->sym->local_offset + i
                                }, init.as.array_lit.at[i]);
                        }
                    } else UNREACHABLE;
                }
                break;
            }
            case TYPE_INT:
            case TYPE_BOOL: {
                if (!sv_is_null(s->sym->top_level_path)) {
                    assert(!s->initializer);
                    for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                        VEC_PUSH(aw->wir->g_ints, ((Qualifier){
                            .path = s->sym->top_level_path,
                            .name = s->name,
                            .id = i
                        }), aw->arena);
                    }
                    break;
                }

                s->sym->local_offset = new_local_int(aw, s->sym->type.array_len);
                if (s->initializer) {
                    RetVal init = visit_Expr(aw, s->initializer);

                    if (init.rv == RV_WOP) {
                        WIROperand elem = init.as.wop;
            
                        for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                            emit_binop(aw,
                                (WIROperand){
                                    .kind = OPKIND_LOCAL_INT,
                                    .as.offset = s->sym->local_offset + i
                                },
                                WIR_ASSIGN_EQ, elem, WIR_IMM_I(0), WIR_BINOP_ADD);
            
                            if (op_is_local(elem))
                                elem.as.offset++;
                            else
                                elem.as.global.id++;
            
                        }
                    } else if (init.rv == RV_DBFIELD) {
                        for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                            emit_load(aw,
                                (WIROperand){
                                    .kind = OPKIND_LOCAL_INT,
                                    .as.offset = s->sym->local_offset + i
                                }, WIR_ASSIGN_EQ, init, i);
                        }
                    } else if (init.rv == RV_ARRAYLIT) {
                        assert((size_t)s->sym->type.array_len == init.as.array_lit.count);
                        for (int32_t i = 0; i < s->sym->type.array_len; i++) {
                            emit_binop(aw,
                                (WIROperand){
                                    .kind = OPKIND_LOCAL_INT,
                                    .as.offset = s->sym->local_offset + i
                                },
                                WIR_ASSIGN_EQ, init.as.array_lit.at[i], WIR_IMM_I(0), WIR_BINOP_ADD);
                        }
                    } else UNREACHABLE;
                }
                break;
            }
            default: UNREACHABLE;
            }

            return;
        }

        switch (s->sym->type.basetype) {
        case TYPE_STR: {
            if (!sv_is_null(s->sym->top_level_path)) {
                assert(!s->initializer);
                VEC_PUSH(aw->wir->g_strs, ((Qualifier){
                    .path = s->sym->top_level_path,
                    .name = s->name,
                    .id = 0
                }), aw->arena);
                break;
            }

            s->sym->local_offset = new_local_str(aw, 1);
            if (s->initializer) {
                WIROperand init = eval(aw, s->initializer);
                emit_str(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL_STR,
                        .as.offset = s->sym->local_offset
                    },
                    init);
            }
            break;
        }
        case TYPE_INT:
        case TYPE_BOOL: {
            if (!sv_is_null(s->sym->top_level_path)) {
                assert(!s->initializer);
                VEC_PUSH(aw->wir->g_ints, ((Qualifier){
                    .path = s->sym->top_level_path,
                    .name = s->name,
                    .id = 0
                }), aw->arena);
                break;
            }

            s->sym->local_offset = new_local_int(aw, 1);
            if (s->initializer) {
                WIROperand init = eval(aw, s->initializer);
                emit_binop(aw,
                    (WIROperand){
                        .kind = OPKIND_LOCAL_INT,
                        .as.offset = s->sym->local_offset
                    },
                    WIR_ASSIGN_EQ, init, WIR_IMM_I(0), WIR_BINOP_ADD);
            }
            break;
        }
        default: UNREACHABLE;
        }
        return;
    }
    case NODE_StmtCevDecl: {
        StmtCevDecl *s = (StmtCevDecl *)stmt;
        
        VEC_PUSH(aw->wir->g_cevs,
            ((WIRCev){
                .qualifier = {
                    .path = aw->current_module->source->path,
                    .name = s->name,
                    .id = 0
                },
                .loc = stmt->tok.loc,
                .insts = VEC_EMPTY,
                .n_temp_ints = 0,
                .n_temp_strs = 0,
                .is_exaddr = s->is_exaddr,
            }), aw->arena);

        open_frame(aw);
        for (size_t i = 0; i < s->params.count; i++)
            visit_Stmt(aw, (Stmt *)s->params.at[i]);
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
            RetVal ret = visit_Expr(aw, s->expr);
            assert(ret.rv == RV_WOP);
            emit_return_val(aw, ret.as.wop);
        } else {
            emit_simple(aw, _WIRInst_ReturnVoid);
        }

        return;
    }
    case NODE_StmtIf: {
        StmtIf *s = (StmtIf *)stmt;
        RetVal cond = visit_Expr(aw, s->condition);
        assert(cond.rv == RV_WOP);

        emit_if_begin(aw, cond.as.wop);

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
            RetVal count = visit_Expr(aw, s->count);
            assert(count.rv == RV_WOP);

            WIRInst_LoopBeginN *inst;
            ALLOC_WIR(inst, WIRInst_LoopBeginN,
                (WIRInst_LoopBeginN){ .count = count.as.wop });
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
            RetVal cond = visit_Expr(aw, s->condition);
            assert(cond.rv == RV_WOP);
            
            WIROperand tmp_cond = tmp_int(aw);
            emit_binop(aw, tmp_cond, WIR_ASSIGN_EQ, cond.as.wop, WIR_IMM_I(1), WIR_BINOP_XOR);

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

        RetVal right_bound = visit_Expr(aw, s->right_bound);
        assert(right_bound.rv == RV_WOP);
        
        WIROperand loop_count = tmp_int(aw);
        emit_binop(aw, loop_count, WIR_ASSIGN_EQ, right_bound.as.wop, iterator, WIR_BINOP_SUB);
        
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

        for (size_t i = 0; i < s->int_operands.count; i++) {
            RetVal rv = visit_Expr(aw, s->int_operands.at[i]);
            assert(rv.rv == RV_WOP);

            VEC_PUSH(iargs, rv.as.wop, aw->arena);
        }
        for (size_t i = 0; i < s->str_operands.count; i++) {
            RetVal rv = visit_Expr(aw, s->str_operands.at[i]);
            assert(rv.rv == RV_WOP);
            
            VEC_PUSH(sargs, rv.as.wop, aw->arena);
        }

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
    case NODE_StmtInc: {
        StmtInc *s = (StmtInc *)stmt;
        RetVal left = visit_Expr(aw, s->expr);
        assert(left.rv == RV_WOP);
        emit_binop(aw, left.as.wop, WIR_ASSIGN_ADD, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
        return;
    }
    case NODE_StmtDec: {
        StmtDec *s = (StmtDec *)stmt;
        RetVal left = visit_Expr(aw, s->expr);
        assert(left.rv == RV_WOP);
        emit_binop(aw, left.as.wop, WIR_ASSIGN_SUB, WIR_IMM_I(1), WIR_IMM_I(0), WIR_BINOP_ADD);
        return;
    }
    case NODE_StmtDBDataDecl: {
        StmtDBDataDecl *s = (StmtDBDataDecl *)stmt;

        for (size_t i = 0; i < aw->wir->g_udbs.count; i++) {
            WIRDB *udb = &aw->wir->g_udbs.at[i];
            if (sv_equals(udb->qualifier.path, s->dbtype_sym->top_level_path)
                && sv_equals(udb->qualifier.name, s->db_type.text)) {
                visit_db_data(aw, s->data, udb, s->data_sym->local_offset);
                return;
            }
        }

        UNREACHABLE;
        return;
    }
    case NODE_StmtDefDB:
    case NODE_StmtDBTypeDecl:
        // Already handled in first pass.
        break;
    }
}

static void rearrange_db_types(Arena *arena, VEC_WIRDB *vec, StmtDefDB *def) {
    // Currently, it's not actually possible to have two DB types under the same DB
    // with the same name, since there's no module namespacing.
    // Therefore, things can just be resolved by name.

    VEC_WIRDB out = VEC_EMPTY;
    for (size_t i = 0; i < def->db_types.count; i++) {
        bool found = false;
        for (size_t j = 0; j < vec->count; j++) {
            if (!sv_equals(def->db_types.at[i].text, vec->at[j].qualifier.name))
                continue; 

            found = true;
            VEC_PUSH(out, vec->at[j], arena);
            break;
        }
        assert(found);
    }

    *vec = out;
}

WIR ast2wir_pass(VEC_Module *modules, Arena *arena) {
    Ast2Wir aw = (Ast2Wir){
        .arena = arena,
        .wir = arena_alloc_assert(arena, sizeof(WIR)),
        .local_frames = VEC_EMPTY,
    };
    wir_init(aw.wir);

    StmtDefDB *udb_def = NULL;
    StmtDefDB *cdb_def = NULL;

    // Do a pass to find all DB types and defs.
    for (size_t i = 0; i < modules->count; i++) {
        aw.current_module = &modules->at[i];
        for (size_t j = 0; j < aw.current_module->ast->stmts.count; j++) {
            Stmt *stmt = aw.current_module->ast->stmts.at[j];
            switch (stmt->kind) {
                case NODE_StmtDBTypeDecl: {
                    StmtDBTypeDecl *s = (StmtDBTypeDecl *)stmt;
                    VEC_WIRDB *g = s->db.type == TOK_UDBTYPE ?
                        &aw.wir->g_udbs : &aw.wir->g_cdbs;

                    WIRDB db = (WIRDB){
                        .qualifier = {
                            .path = aw.current_module->source->path,
                            .name = s->name,
                            .id = 0
                        },
                        .fields = VEC_EMPTY,
                        .data = VEC_EMPTY
                    };
                    
                    for (size_t k = 0; k < s->fields.count; k++)
                        visit_db_field_decl(&aw, s->fields.at[k], &db);

                    for (size_t k = 0; k < s->data.count; k++) {
                        VEC_PUSH(db.data, ((WIRData){.values = VEC_EMPTY}), aw.arena);
                        visit_db_data(&aw, s->data.at[k], &db, k);
                    }

                    VEC_PUSH(*g, db, aw.arena);
                    break;
                }
                case NODE_StmtDefDB: {
                    StmtDefDB *s = (StmtDefDB *)stmt;
                    if (s->db == DB_UDB)
                        udb_def = s;
                    else cdb_def = s;
                    break;
                }
                default: break;
            }
        }
    }

    // Order DBs by def.
    if (udb_def) rearrange_db_types(arena, &aw.wir->g_udbs, udb_def);
    if (cdb_def) rearrange_db_types(arena, &aw.wir->g_cdbs, cdb_def);

    // Compile all statements into IR.
    for (size_t i = 0; i < modules->count; i++) {
        aw.current_module = &modules->at[i];
        for (size_t j = 0; j < aw.current_module->ast->stmts.count; j++) {
            visit_Stmt(&aw, aw.current_module->ast->stmts.at[j]);
            assert(aw.local_frames.count == 0);
        }
    }

    return *aw.wir;
}