#include "wir.h"
#include "sdb.h"
#include "error.h"

#define CSELF_BASE 1600000
#define CSELF_INT_MAX  1600099
#define CSELF_STR_BASE 1600005
#define CSELF_STR_MAX  1600009

#define RC_THRESHOLD 1000000
#define NORMAL_VAR_BASE 2000000
#define STRING_VAR_BASE 3000000
#define CEV_BASE 500000

#define SDB_STRING_VAR_DBTYPE 4
#define SDB_NORMAL_VAR_DBTYPE 14

typedef struct WIRCompiler {
    WIR *wir;

    // Information about the current common event.
    WIRCev *wcev;
    CommonEvent *cev;
    uint8_t indent;

    // Where the 0 addresses currently are for exaddr common events,
    // relative to the first global of each type.
    size_t exaddr_int_base;
    size_t exaddr_str_base;

    GameData gd;
    Arena *arena;
    bool had_error;
    bool panic_mode;
} WIRCompiler;

static void wc_error(WIRCompiler *wc, Location loc, StringView msg) {
    wc->had_error = true;
    if (wc->panic_mode) return;

    wc->panic_mode = true;
    error(loc, msg);
}

static size_t get_sdb_int_top(WIRCompiler *wc) {
    return wc->gd.sdb.at[SDB_NORMAL_VAR_DBTYPE].data.count;
}

static size_t get_sdb_str_top(WIRCompiler *wc) {
    return wc->gd.sdb.at[SDB_STRING_VAR_DBTYPE].data.count;
}

static size_t push_sdb_int(WIRCompiler *wc, StringView name) {
    VEC_DBData *v = &wc->gd.sdb.at[SDB_NORMAL_VAR_DBTYPE].data;

    VEC_PUSH(*v, ((DBData){.name = name, .values = VEC_EMPTY}), wc->arena);

    if (v->count > 10000) {
        fprintf(stderr, "Fatal error: No more space in integer globals.");
        exit(1);
    }

    return v->count - 1;
}

static size_t push_sdb_str(WIRCompiler *wc, StringView name) {
    VEC_DBData *v = &wc->gd.sdb.at[SDB_STRING_VAR_DBTYPE].data;

    VEC_PUSH(*v, ((DBData){.name = name, .values = VEC_EMPTY}), wc->arena);

    if (v->count > 10000) {
        fprintf(stderr, "Fatal error: No more space in integer globals.");
        exit(1);
    }

    return v->count - 1;
}

bool op_is_string(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_LOCAL_STR:
        case OPKIND_TEMP_STR:
        case OPKIND_GLOBAL_STR:
            return true;
        case OPKIND_IMM_INT:
        case OPKIND_TEMP_INT:
        case OPKIND_LOCAL_INT:
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDBTYPE:
        case OPKIND_GLOBAL_CDBTYPE:
            return false;
        case OPKIND_DBDATA:
        case OPKIND_DBFIELD:
            UNREACHABLE;
            return false;
    }

    UNREACHABLE;
    return false;
}

static bool op_is_strlit(WIROperand wop) {
    return wop.kind == OPKIND_IMM_STR
        || wop.kind == OPKIND_INTERP;
}

static size_t i_temp(WIRCev *wcev) {
    return wcev->n_temp_ints++;
}

static void insert_inst(Arena *arena, WIRCev *wcev, size_t pos, WIRInst *inst) {
    assert(pos <= wcev->insts.count);

    if (pos == wcev->insts.count) {
        VEC_PUSH(wcev->insts, inst, arena);
        return;
    }

    // If `pos` is strictly greater than the vector length, there must
    // be at least one element in the vector.
    size_t last = wcev->insts.count - 1;

    VEC_PUSH(wcev->insts, NULL, arena);
    for (size_t i = last; i >= pos; i--) {
        wcev->insts.at[i + 1] = wcev->insts.at[i];
    }

    wcev->insts.at[pos] = inst;
}

// If applicable, patches the operand to avoid reference conversion,
// inserting a new binop instruction at the given position.
// The input position is updated to jump over any newly-inserted instructions.
static void disable_rc(Arena *arena, WIRCev *wcev, size_t *inst_pos, WIROperand *wop) {
    if (wop->kind != OPKIND_IMM_INT) return;
    if (wop->as.imm_int < RC_THRESHOLD) return;
    
    size_t temp = i_temp(wcev);

    WIRInst_Binop *inst = arena_alloc_assert(arena, sizeof(WIRInst_Binop));

    *inst = (WIRInst_Binop){
        .base.kind = _WIRInst_Binop,
        .dest = { .kind = OPKIND_TEMP_INT, .as.offset = temp },
        .op = WIR_BINOP_SUB,
        .a = { .kind = OPKIND_IMM_INT, .as.imm_int = 0 },
        .b = { .kind = OPKIND_IMM_INT, .as.imm_int = -wop->as.imm_int}
    };

    insert_inst(arena, wcev, *inst_pos, (WIRInst *)inst);
    *wop = (WIROperand){
        .kind = OPKIND_TEMP_INT,
        .as.offset = temp
    };

    // There is no need to analyze the newly-inserted instruction.
    *inst_pos++;
}

// For a given `WIRCev`, update integer immediates to use a temporary if they would
// otherwise be treated as a reference.
static void disable_rc_pass(Arena *arena, WIRCev *wcev) {
    for (size_t i = 0; i < wcev->insts.count; i++) {
        WIRInst *wirinst = wcev->insts.at[i];
        switch (wirinst->kind) {
        case _WIRInst_NOP:
        case _WIRInst_PushIntN:
        case _WIRInst_PushStrN:
        case _WIRInst_PopIntN:
        case _WIRInst_PopStrN:
        case _WIRInst_StrAssign:
        case _WIRInst_Cmd:
        case _WIRInst_ReturnVoid:
        case _WIRInst_Continue:
        case _WIRInst_Break:
        case _WIRInst_LoopEnd:
        case _WIRInst_Else:
        case _WIRInst_IfEnd:
        case _WIRInst_Label:
        case _WIRInst_Goto:
        case _WIRInst_LoopBegin:
            break;
        case _WIRInst_Binop: {
            WIRInst_Binop *inst = (WIRInst_Binop *)wirinst;
            disable_rc(arena, wcev, &i, &inst->a);
            disable_rc(arena, wcev, &i, &inst->b);
            break;
        }
        case _WIRInst_Compare: {
            WIRInst_Compare *inst = (WIRInst_Compare *)wirinst;
            disable_rc(arena, wcev, &i, &inst->a);
            disable_rc(arena, wcev, &i, &inst->b);
            break;
        }
        case _WIRInst_IfBegin: {
            WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wirinst;
            disable_rc(arena, wcev, &i, &inst->cond);
            break;
        }
        case _WIRInst_IfBeginOp: {
            WIRInst_IfBeginOp *inst = (WIRInst_IfBeginOp *)wirinst;
            disable_rc(arena, wcev, &i, &inst->a);
            disable_rc(arena, wcev, &i, &inst->b);
            break;
        }
        case _WIRInst_LoopBeginN: {
            WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wirinst;
            disable_rc(arena, wcev, &i, &inst->count);
            break;
        }
        case _WIRInst_Call: {
            WIRInst_Call *inst = (WIRInst_Call *)wirinst;
            // Disabling reference conversion for the destination
            // is unnecessary, assuming that you can't store a
            // value into an immediate.
            for (size_t arg = 0; arg < inst->args.count; arg++)
                disable_rc(arena, wcev, &arg, &inst->args.at[arg]);
            break;
        }
        case _WIRInst_ReturnVal: {
            WIRInst_ReturnVal *inst = (WIRInst_ReturnVal *)wirinst;
            disable_rc(arena, wcev, &i, &inst->val);
            break;
        }
        case _WIRInst_DBLoad: {
            WIRInst_DBLoad *inst = (WIRInst_DBLoad *)wirinst;
            disable_rc(arena, wcev, &i, &inst->db_type);
            disable_rc(arena, wcev, &i, &inst->db_data);
            disable_rc(arena, wcev, &i, &inst->db_field);
            break;
        }
        case _WIRInst_DBStore: {
            WIRInst_DBStore *inst = (WIRInst_DBStore *)wirinst;
            disable_rc(arena, wcev, &i, &inst->db_type);
            disable_rc(arena, wcev, &i, &inst->db_data);
            disable_rc(arena, wcev, &i, &inst->db_field);
            break;
        }
        }
    }
}

static bool qualifier_eq(Qualifier a, Qualifier b) {
    return sv_equals(a.path, b.path)
        && sv_equals(a.name, b.name);
}

// Resolves a non-string `WIROperand` into a concrete integer value.
// For "variables" and common events, this is their reference value.
// For database types, this is just the type ID, since it is relatively
// rare that the reference value of a database is used in a command.
//
// Assumes reference conversion has already been disabled.
static int32_t resolve(WIRCompiler *wc, WIROperand wop) {
    assert(wop.kind != OPKIND_IMM_STR && wop.kind != OPKIND_INTERP);
    
    switch (wop.kind) {
    case OPKIND_IMM_INT: {
        return wop.as.imm_int;
    }
    case OPKIND_LOCAL_INT: {
        int32_t ref = wc->wcev->local_int_map.at[wop.as.offset];
        assert(ref >= RC_THRESHOLD);
        return ref;
    }
    case OPKIND_LOCAL_STR: {
        int32_t ref = wc->wcev->local_str_map.at[wop.as.offset];
        assert(ref >= RC_THRESHOLD);
        return ref;
    }
    case OPKIND_TEMP_INT: {
        int32_t ref = wc->wcev->temp_int_map.at[wop.as.offset];
        assert(ref >= RC_THRESHOLD);
        return ref;
    }
    case OPKIND_TEMP_STR: {
        int32_t ref = wc->wcev->temp_str_map.at[wop.as.offset];
        assert(ref >= RC_THRESHOLD);
        return ref;
    }
    case OPKIND_GLOBAL_INT: {
        for (size_t i = 0; i < wc->wir->g_ints.count; i++)
            if (qualifier_eq(wop.as.global, wc->wir->g_ints.at[i]))
                return NORMAL_VAR_BASE + i;
        break;
    }
    case OPKIND_GLOBAL_STR: {
        for (size_t i = 0; i < wc->wir->g_strs.count; i++)
            if (qualifier_eq(wop.as.global, wc->wir->g_strs.at[i]))
                return STRING_VAR_BASE + i;
        break;
    }
    case OPKIND_GLOBAL_CEV: {
        for (size_t i = 0; i < wc->wir->g_cevs.count; i++)
            if (qualifier_eq(wop.as.global, wc->wir->g_cevs.at[i].qualifier))
                return CEV_BASE + i;
        break;
    }
    case OPKIND_GLOBAL_UDBTYPE: {
        for (size_t i = 0; i < wc->wir->g_udbs.count; i++)
            if (qualifier_eq(wop.as.global, wc->wir->g_udbs.at[i].qualifier))
                return i;
        break;
    }
    case OPKIND_GLOBAL_CDBTYPE: {
        for (size_t i = 0; i < wc->wir->g_cdbs.count; i++)
            if (qualifier_eq(wop.as.global, wc->wir->g_cdbs.at[i].qualifier))
                return i;
        break;
    }
    
    case OPKIND_IMM_STR:
    case OPKIND_INTERP:
    case OPKIND_DBDATA:
    case OPKIND_DBFIELD:
        UNREACHABLE;
    }

    UNREACHABLE;
    return 0;
}

static void interp_int_var(char *buf, size_t buf_size, bool is_exaddr, int32_t ref) {
    if (is_exaddr) {
        snprintf(buf, buf_size, "\\v[%zu]", ref - NORMAL_VAR_BASE);
    } else {
        snprintf(buf, buf_size, "\\cself[%zu]", ref - CSELF_BASE);
    }
}

static void interp_str_var(char *buf, size_t buf_size, bool is_exaddr, int32_t ref) {
    if (is_exaddr) {
        snprintf(buf, buf_size, "\\s[%zu]", ref - STRING_VAR_BASE);
    } else {
        snprintf(buf, buf_size, "\\cself[%zu]", ref - CSELF_BASE);
    }
}

static StringView interpolate(WIRCompiler *wc, WIROperand wop) {
    assert(op_is_strlit(wop));

    if (wop.kind == OPKIND_IMM_STR)
        return wop.as.imm_str;

    assert(wop.kind == OPKIND_INTERP);

    StringView out = SV("");
    for (size_t i = 0; i < wop.as.interp.count; i++) {
        WIROperand frag = wop.as.interp.at[i];

        StringView next = SV("");
        char buf[sizeof(int32_t) * 8 + 1];
        switch (frag.kind) {
        case OPKIND_IMM_STR:
            next = frag.as.imm_str;
            break;
        case OPKIND_INTERP:
            next = interpolate(wc, frag);
            break;
        case OPKIND_IMM_INT: {
            snprintf(buf, sizeof(buf), "%d", frag.as.imm_int);
            next = to_sv(buf);
            break;
        }
        case OPKIND_LOCAL_INT: {
            int32_t ref = wc->wcev->local_int_map.at[frag.as.offset];
            interp_int_var(buf, sizeof(buf), wc->wcev->is_exaddr, ref);
            next = to_sv(buf);
            break;
        }
        case OPKIND_LOCAL_STR: {
            int32_t ref = wc->wcev->local_str_map.at[frag.as.offset];
            interp_str_var(buf, sizeof(buf), wc->wcev->is_exaddr, ref);
            next = to_sv(buf);
            break;
        }
        case OPKIND_TEMP_INT: {
            int32_t ref = wc->wcev->temp_int_map.at[frag.as.offset];
            interp_int_var(buf, sizeof(buf), wc->wcev->is_exaddr, ref);
            next = to_sv(buf);
            break;
        }
        case OPKIND_TEMP_STR: {
            int32_t ref = wc->wcev->temp_str_map.at[frag.as.offset];
            interp_str_var(buf, sizeof(buf), wc->wcev->is_exaddr, ref);
            next = to_sv(buf);
            break;
        }
        case OPKIND_GLOBAL_INT: {
            for (size_t j = 0; j < wc->wir->g_ints.count; j++) {
                if (qualifier_eq(frag.as.global, wc->wir->g_ints.at[j])) {
                    snprintf(buf, sizeof(buf), "\\v[%zu]", j);
                    next = to_sv(buf);
                }
            }
            UNREACHABLE;
            break;
        }
        case OPKIND_GLOBAL_STR: {
            for (size_t j = 0; j < wc->wir->g_strs.count; j++) {
                if (qualifier_eq(frag.as.global, wc->wir->g_strs.at[j])) {
                    snprintf(buf, sizeof(buf), "\\s[%zu]", j);
                    next = to_sv(buf);
                }
            }
            UNREACHABLE;
            break;
        }
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDBTYPE:
        case OPKIND_GLOBAL_CDBTYPE:
            next = frag.as.global.name;
            break;
        case OPKIND_DBDATA:
        case OPKIND_DBFIELD:
            UNREACHABLE;
            break;
        }

        out = sv_concat(wc->arena, out, next);
    }

    return out;
}

typedef struct {
    size_t id;
    size_t start;
    size_t end;
} Interval;
VEC_DEF(Interval);

static int cb_interval_start_asc(const void *a, const void *b) {
    const Interval *it_a = a;
    const Interval *it_b = b;

    if (it_a->start < it_b->start) return -1;
    if (it_a->start > it_b->start) return 1;
    return 0;
}

static int cb_interval_end_desc(const void *a, const void *b) {
    const Interval *it_a = a;
    const Interval *it_b = b;

    if (it_a->end < it_b->end) return 1;
    if (it_a->end > it_b->end) return -1;
    return 0;
}

static void update_interval(VEC_Interval *i_its, VEC_Interval *s_its, size_t inst, WIROperand wop) {
    if (wop.kind == OPKIND_INTERP) {
        for (size_t i = 0; i < wop.as.interp.count; i++)
            update_interval(i_its, s_its, inst, wop.as.interp.at[i]);
        return;
    }

    if (wop.kind != OPKIND_TEMP_INT && wop.kind != OPKIND_TEMP_STR) return;

    Interval *it;
    if (wop.kind == OPKIND_TEMP_INT) {
        assert(wop.as.offset < i_its->count);
        it = &i_its->at[wop.as.offset];
    } else {
        assert(wop.as.offset < s_its->count);
        it = &s_its->at[wop.as.offset];
    }

    if (it->start == -1) it->start = inst;
    it->end = inst;
}

static void update_temp_map(WIRCev *wcev, int32_t i_top, int32_t s_top, WIROperand wop) {
    if (wop.kind == OPKIND_TEMP_INT) {
        int32_t *v = &wcev->temp_int_map.at[wop.as.offset];

        // If temporary has already been given a concrete address, skip.
        if (*v >= RC_THRESHOLD) return;

        if (wcev->is_exaddr) {
            *v += NORMAL_VAR_BASE + i_top;
        } else {
            *v += CSELF_BASE + i_top;

            // If we have reached the string space, jump over it.
            if (*v >= CSELF_STR_BASE)
                *v += CSELF_STR_BASE - CSELF_BASE;
        }

    } else if (wop.kind == OPKIND_TEMP_STR) {
        int32_t *v = &wcev->temp_str_map.at[wop.as.offset];

        if (*v >= RC_THRESHOLD) return;

        if (wcev->is_exaddr) {
            *v += STRING_VAR_BASE + s_top;
        } else {
            *v += CSELF_STR_BASE + s_top;
        }
    }
}

// Allocate offsets to temporaries.
// Essentially, this is linear register allocation but with an
// infinite number of physical registers.
// (https://en.wikipedia.org/wiki/Register_allocation#Linear_scan)
static VEC_int32_t reg_alloc(Arena *arena, VEC_Interval *its, size_t n_temps) {
    VEC_int32_t map = VEC_EMPTY;
    for (size_t i = 0; i < n_temps; i++)
        VEC_PUSH(map, 0, arena);
    
    // Whether or not a register is active.
    VEC_DEF(bool);
    VEC_bool regs = VEC_EMPTY;
    for (size_t i = 0; i < n_temps; i++)
        VEC_PUSH(regs, false, arena);
    
    VEC_Interval i_active = VEC_EMPTY;

    qsort(its->at, its->count,
        sizeof(its->at[0]), cb_interval_start_asc);

    for (size_t i = 0; i < its->count; i++) {
        qsort(i_active.at, i_active.count,
            sizeof(i_active.at[0]), cb_interval_end_desc);
        
        for (size_t j = 0; j < i_active.count;) {
            if (i_active.at[j].end >= its->at[i].start)
                break;

            VEC_REMOVE(i_active, j);
            regs.at[i_active.at[j].id] = false;
        }

        assert(i_active.count <= regs.count);

        for (size_t reg = 0; reg < regs.count; reg++) {
            if (regs.at[reg]) continue;
            map.at[its->at[i].id] = reg;
            regs.at[reg] = true;
            break;
        }
        VEC_PUSH(i_active, its->at[i], arena);
    }

    return map;
}

// Assigns concrete addresses to locals and temporaries.
static void addr_alloc_pass(WIRCompiler *wc, WIRCev *wcev) {
    // Compute liveness intervals of all temporaries.
    size_t n_i_temps = wcev->n_temp_ints;
    size_t n_s_temps = wcev->n_temp_strs;

    VEC_Interval i_its = VEC_EMPTY;
    for (size_t i = 0; i < n_i_temps; i++)
        VEC_PUSH(i_its, ((Interval){ .id = i,
            .start = (size_t)-1, .end = (size_t)-1 }), wc->arena);
    
    VEC_Interval s_its = VEC_EMPTY;
    for (size_t i = 0; i < n_s_temps; i++)
        VEC_PUSH(s_its, ((Interval){ .id = i,
            .start = (size_t)-1, .end = (size_t)-1 }), wc->arena);

    for (size_t i = 0; i < wcev->insts.count; i++) {
        WIRInst *wirinst = wcev->insts.at[i];
        switch (wirinst->kind) {
        case _WIRInst_NOP:
        case _WIRInst_PushIntN:
        case _WIRInst_PushStrN:
        case _WIRInst_PopIntN:
        case _WIRInst_PopStrN:
        case _WIRInst_Cmd:
        case _WIRInst_ReturnVoid:
        case _WIRInst_Continue:
        case _WIRInst_Break:
        case _WIRInst_LoopEnd:
        case _WIRInst_Else:
        case _WIRInst_IfEnd:
        case _WIRInst_LoopBegin:
            break;
        case _WIRInst_StrAssign: {
            WIRInst_StrAssign *inst = (WIRInst_StrAssign *)wirinst;
            update_interval(&i_its, &s_its, i, inst->dest);
            update_interval(&i_its, &s_its, i, inst->src);
            break;
        }
        case _WIRInst_Label: {
            WIRInst_Label *inst = (WIRInst_Label *)wirinst;
            update_interval(&i_its, &s_its, i, inst->name);
            break;
        }
        case _WIRInst_Goto: {
            WIRInst_Goto *inst = (WIRInst_Goto *)wirinst;
            update_interval(&i_its, &s_its, i, inst->name);
            break;
        }
        case _WIRInst_Binop: {
            WIRInst_Binop *inst = (WIRInst_Binop *)wirinst;
            update_interval(&i_its, &s_its, i, inst->dest);
            update_interval(&i_its, &s_its, i, inst->a);
            update_interval(&i_its, &s_its, i, inst->b);
            break;
        }
        case _WIRInst_Compare: {
            WIRInst_Compare *inst = (WIRInst_Compare *)wirinst;
            update_interval(&i_its, &s_its, i, inst->dest);
            update_interval(&i_its, &s_its, i, inst->a);
            update_interval(&i_its, &s_its, i, inst->b);
            break;
        }
        case _WIRInst_IfBegin: {
            WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wirinst;
            update_interval(&i_its, &s_its, i, inst->cond);
            break;
        }
        case _WIRInst_IfBeginOp: {
            WIRInst_IfBeginOp *inst = (WIRInst_IfBeginOp *)wirinst;
            update_interval(&i_its, &s_its, i, inst->a);
            update_interval(&i_its, &s_its, i, inst->b);
            break;
        }
        case _WIRInst_LoopBeginN: {
            WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wirinst;
            update_interval(&i_its, &s_its, i, inst->count);
            break;
        }
        case _WIRInst_Call: {
            WIRInst_Call *inst = (WIRInst_Call *)wirinst;
            update_interval(&i_its, &s_its, i, inst->dest);
            for (size_t arg = 0; arg < inst->args.count; arg++)
                update_interval(&i_its, &s_its, i, inst->args.at[arg]);
            break;
        }
        case _WIRInst_ReturnVal: {
            WIRInst_ReturnVal *inst = (WIRInst_ReturnVal *)wirinst;
            update_interval(&i_its, &s_its, i, inst->val);
            break;
        }
        case _WIRInst_DBLoad: {
            WIRInst_DBLoad *inst = (WIRInst_DBLoad *)wirinst;
            update_interval(&i_its, &s_its, i, inst->dst);
            update_interval(&i_its, &s_its, i, inst->db_type);
            update_interval(&i_its, &s_its, i, inst->db_data);
            update_interval(&i_its, &s_its, i, inst->db_field);
            break;
        }
        case _WIRInst_DBStore: {
            WIRInst_DBStore *inst = (WIRInst_DBStore *)wirinst;
            update_interval(&i_its, &s_its, i, inst->src);
            update_interval(&i_its, &s_its, i, inst->db_type);
            update_interval(&i_its, &s_its, i, inst->db_data);
            update_interval(&i_its, &s_its, i, inst->db_field);
            break;
        }
        }
    }

    // If a temporary wasn't found, remove its interval.
    for (size_t i = 0; i < i_its.count;) {
        if (i_its.at[i].start != -1) {
            i++;
            continue;
        }
        VEC_REMOVE(i_its, i);
    }
    for (size_t i = 0; i < s_its.count;) {
        if (s_its.at[i].start != -1) {
            i++;
            continue;
        }
        VEC_REMOVE(s_its, i);
    }

    // Allocate virtual offsets to temporaries (as in, from 0).
    wcev->temp_int_map = reg_alloc(wc->arena, &i_its, n_i_temps);
    wcev->temp_str_map = reg_alloc(wc->arena, &s_its, n_s_temps);

    // Now do one more pass of the code, keeping track of the
    // state of the compile-time stack this time to assign concrete
    // addresses to operands.
    //
    // For locals, addresses are assigned fairly straightforwardly,
    // with the exception that CSelf5~9 need to be avoided when
    // allocating integers without exaddr. If a push instruction
    // allocates a block of integers such that they would flow into
    // that space, skip ahead to CSelf10 instead. This is a bit of a waste,
    // but losing a few CSelfs doesn't matter that much in the long run.
    //
    // For temporaries, addresses are assigned such that they are
    // always right above the local stack. Again, this needs to take
    // into consideration the case that an integer temporary could be
    // allocated into CSelf5~9, and skip over in that case.
    wcev->local_int_map = (VEC_int32_t)VEC_EMPTY;
    wcev->local_str_map = (VEC_int32_t)VEC_EMPTY;

    int32_t i_top = 0;
    int32_t s_top = 0;

    for (size_t i = 0; i < wcev->insts.count; i++) {
        WIRInst *wirinst = wcev->insts.at[i];
        switch (wirinst->kind) {
        case _WIRInst_PushIntN: {
            WIRInst_PushIntN *inst = (WIRInst_PushIntN *)wirinst;
            assert(inst->n > 0);

            if (wcev->is_exaddr) {
                for (size_t j = 0; j < inst->n; j++)
                    VEC_PUSH(wcev->local_int_map,
                        NORMAL_VAR_BASE + wc->exaddr_int_base + j,
                        wc->arena);

                i_top += inst->n;
                break;
            }

            int32_t range_start = CSELF_BASE + i_top;
            
            // If range has reached the string space, jump over it.
            if (range_start >= CSELF_STR_BASE) {
                range_start += CSELF_STR_BASE - CSELF_BASE;
            }

            if (range_start + inst->n - 1 > CSELF_INT_MAX)
                wc_error(wc, wcev->loc, SV("Ran out of integer CSelfs to allocate. " 
                    "Try marking the common event as 'exaddr'."));

            for (size_t j = 0; j < inst->n; j++)
                VEC_PUSH(wcev->local_int_map, range_start + j, wc->arena);

            i_top += inst->n;

            break;
        }
        case _WIRInst_PushStrN: {
            WIRInst_PushStrN *inst = (WIRInst_PushStrN *)wirinst;
            assert(inst->n > 0);

            if (wcev->is_exaddr) {
                for (size_t j = 0; j < inst->n; j++)
                    VEC_PUSH(wcev->local_str_map,
                        STRING_VAR_BASE + wc->exaddr_str_base + j,
                        wc->arena);

                s_top += inst->n;
                break;
            }

            int32_t range_start = CSELF_STR_BASE + s_top;
            int32_t range_end = range_start + inst->n - 1;
            
            if (range_end > CSELF_INT_MAX)
                wc_error(wc, wcev->loc, SV("Ran out of integer CSelfs to allocate. " 
                    "Try marking the common event as 'exaddr'."));

            for (size_t j = 0; j < inst->n; j++)
                VEC_PUSH(wcev->local_str_map, range_start + j, wc->arena);

            s_top += inst->n;

            break;
        }
        case _WIRInst_PopIntN: {
            WIRInst_PopIntN *inst = (WIRInst_PopIntN *)wirinst;
            i_top -= inst->n;
            break;
        }
        case _WIRInst_PopStrN: {
            WIRInst_PopStrN *inst = (WIRInst_PopStrN *)wirinst;
            s_top -= inst->n;
            break;
        }
        case _WIRInst_Binop: {
            WIRInst_Binop *inst = (WIRInst_Binop *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->dest);
            update_temp_map(wcev, i_top, s_top, inst->a);
            update_temp_map(wcev, i_top, s_top, inst->b);
            break;
        }
        case _WIRInst_Compare: {
            WIRInst_Compare *inst = (WIRInst_Compare *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->dest);
            update_temp_map(wcev, i_top, s_top, inst->a);
            update_temp_map(wcev, i_top, s_top, inst->b);
            break;
        }
        case _WIRInst_IfBegin: {
            WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->cond);
            break;
        }
        case _WIRInst_IfBeginOp: {
            WIRInst_IfBeginOp *inst = (WIRInst_IfBeginOp *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->a);
            update_temp_map(wcev, i_top, s_top, inst->b);
            break;
        }
        case _WIRInst_LoopBeginN: {
            WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->count);
            break;
        }
        case _WIRInst_Call: {
            WIRInst_Call *inst = (WIRInst_Call *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->dest);
            for (size_t arg = 0; arg < inst->args.count; arg++)
                update_temp_map(wcev, i_top, s_top, inst->args.at[arg]);
            break;
        }
        case _WIRInst_ReturnVal: {
            WIRInst_ReturnVal *inst = (WIRInst_ReturnVal *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->val);
            break;
        }
        case _WIRInst_DBLoad: {
            WIRInst_DBLoad *inst = (WIRInst_DBLoad *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->dst);
            update_temp_map(wcev, i_top, s_top, inst->db_type);
            update_temp_map(wcev, i_top, s_top, inst->db_data);
            update_temp_map(wcev, i_top, s_top, inst->db_field);
            break;
        }
        case _WIRInst_DBStore: {
            WIRInst_DBStore *inst = (WIRInst_DBStore *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->src);
            update_temp_map(wcev, i_top, s_top, inst->db_type);
            update_temp_map(wcev, i_top, s_top, inst->db_data);
            update_temp_map(wcev, i_top, s_top, inst->db_field);
            break;
        }
        case _WIRInst_StrAssign: {
            WIRInst_StrAssign *inst = (WIRInst_StrAssign *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->dest);
            update_temp_map(wcev, i_top, s_top, inst->src);
            break;
        }
        case _WIRInst_Label: {
            WIRInst_Label *inst = (WIRInst_Label *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->name);
            break;
        }
        case _WIRInst_Goto: {
            WIRInst_Goto *inst = (WIRInst_Goto *)wirinst;
            update_temp_map(wcev, i_top, s_top, inst->name);
            break;
        }
        case _WIRInst_NOP:
        case _WIRInst_Cmd:
        case _WIRInst_ReturnVoid:
        case _WIRInst_Continue:
        case _WIRInst_Break:
        case _WIRInst_LoopEnd:
        case _WIRInst_Else:
        case _WIRInst_IfEnd:
        case _WIRInst_LoopBegin:
            break;
        }
    }

    // Update global exaddr info.
    if (wcev->is_exaddr) {
        wc->exaddr_int_base += i_top;
        wc->exaddr_str_base += s_top;
    }
}

static void push_binop_command(WIRCompiler *wc, int32_t dest, int cmd_assign, int32_t a, int32_t b, int cmd_var_op) {
    VEC_int32_t i_vec = VEC_EMPTY;
    
    VEC_PUSH(i_vec, dest, wc->arena);
    VEC_PUSH(i_vec, a, wc->arena);
    VEC_PUSH(i_vec, b, wc->arena);
    VEC_PUSH(i_vec, cmd_assign | cmd_var_op, wc->arena);

    cev_push_cmd(wc->cev, CMD_VAR, wc->indent, i_vec, (VEC_StringView)VEC_EMPTY);
}

static void compile_binop(WIRCompiler *wc, WIRInst_Binop *inst,
    CommandVarFlag cmd_assign, CommandVarFlag cmd_var_op)
{
    push_binop_command(wc,
        resolve(wc, inst->dest), cmd_assign,
        resolve(wc, inst->a), resolve(wc, inst->b), cmd_var_op);
}

static void push_str_command(WIRCompiler *wc, int32_t dest_ref, WIROperand src) {
    VEC_int32_t int_fields = VEC_EMPTY;
    VEC_StringView str_fields = VEC_EMPTY;
    if (op_is_strlit(src)) {
        VEC_PUSH(int_fields, dest_ref, wc->arena);
        VEC_PUSH(int_fields, 0, wc->arena);
        VEC_PUSH(int_fields, 0, wc->arena);
        VEC_PUSH(str_fields, interpolate(wc, src), wc->arena);
    } else {
        VEC_PUSH(int_fields, dest_ref, wc->arena);
        VEC_PUSH(int_fields, 0, wc->arena);
        VEC_PUSH(int_fields, resolve(wc, src), wc->arena);
    }
    
    cev_push_cmd(wc->cev, CMD_STRING, wc->indent,
        int_fields, str_fields);
}

static void compile_compare(WIRCompiler *wc, WIRInst_Compare *inst,
    CommandIfIntFlag cmd_comp_op)
{
    {
        VEC_int32_t i_vec = VEC_EMPTY;
        VEC_PUSH(i_vec, 1 | 0x10, wc->arena);
        VEC_PUSH(i_vec, resolve(wc, inst->a), wc->arena);
        VEC_PUSH(i_vec, resolve(wc, inst->b), wc->arena);
        VEC_PUSH(i_vec, cmd_comp_op, wc->arena);
        cev_push_cmd(wc->cev, CMD_IF_INT, wc->indent,
            i_vec, (VEC_StringView)VEC_EMPTY);
    }

    {
        VEC_int32_t i_vec = VEC_EMPTY;
        VEC_PUSH(i_vec, 1, wc->arena);

        cev_push_cmd(wc->cev, CMD_BRANCH, wc->indent,
            i_vec, (VEC_StringView)VEC_EMPTY);
    }

    {
        VEC_int32_t i_vec = VEC_EMPTY;
        VEC_PUSH(i_vec, resolve(wc, inst->dest), wc->arena);
        VEC_PUSH(i_vec, 1, wc->arena);
        VEC_PUSH(i_vec, 0, wc->arena);
        VEC_PUSH(i_vec, 0, wc->arena);
        cev_push_cmd(wc->cev, CMD_VAR, wc->indent + 1,
            i_vec, (VEC_StringView)VEC_EMPTY);
    }

    {
        VEC_int32_t i_vec = VEC_EMPTY;
        VEC_PUSH(i_vec, 0, wc->arena);

        cev_push_cmd(wc->cev, CMD_BRANCH_ELSE, wc->indent,
            i_vec, (VEC_StringView)VEC_EMPTY);
    }

    {
        VEC_int32_t i_vec = VEC_EMPTY;
        VEC_PUSH(i_vec, resolve(wc, inst->dest), wc->arena);
        VEC_PUSH(i_vec, 0, wc->arena);
        VEC_PUSH(i_vec, 0, wc->arena);
        VEC_PUSH(i_vec, 0, wc->arena);
        cev_push_cmd(wc->cev, CMD_VAR, wc->indent + 1,
            i_vec, (VEC_StringView)VEC_EMPTY);
    }

    cev_push_simple_cmd(wc->cev, CMD_IF_END, wc->indent);
}

static bool find_else(WIRCev *wcev, size_t head_index) {
    size_t if_depth = 0;
    for (size_t i = head_index + 1; i < wcev->insts.count; i++) {
        switch (wcev->insts.at[i]->kind) {
            case _WIRInst_IfBegin:
            case _WIRInst_IfBeginOp:
                if_depth++;
                break;
            case _WIRInst_Else:
                if (if_depth == 0)
                    return true;
            case _WIRInst_IfEnd:
                if (if_depth == 0)
                    return false;
                if_depth--;
                break;
            default: break;
        }
    }
    UNREACHABLE;
    return false;
}

static void compile_inst(WIRCompiler *wc, size_t index) {
    WIRInst *wi = wc->wcev->insts.at[index];
    switch (wi->kind) {
    case _WIRInst_NOP:
    case _WIRInst_PushIntN:
    case _WIRInst_PushStrN:
    case _WIRInst_PopIntN:
    case _WIRInst_PopStrN:
        break;
    case _WIRInst_Binop: {
        WIRInst_Binop *inst = (WIRInst_Binop *)wi;
        int cmd_assign = 0;
        switch (inst->assign) {
        case WIR_ASSIGN_EQ:  cmd_assign = VAR_ASSIGN_EQ; break;
        case WIR_ASSIGN_ADD: cmd_assign = VAR_ASSIGN_PLUS_EQ; break;
        case WIR_ASSIGN_SUB: cmd_assign = VAR_ASSIGN_MINUS_EQ; break;
        case WIR_ASSIGN_MUL: cmd_assign = VAR_ASSIGN_TIMES_EQ; break;
        case WIR_ASSIGN_DIV: cmd_assign = VAR_ASSIGN_DIV_EQ; break;
        case WIR_ASSIGN_MOD: cmd_assign = VAR_ASSIGN_MOD_EQ; break;
        }

        switch (inst->op) {
        case WIR_BINOP_ADD: compile_binop(wc, inst, cmd_assign, VAR_OP_PLUS); break;
        case WIR_BINOP_SUB: compile_binop(wc, inst, cmd_assign, VAR_OP_MINUS); break;
        case WIR_BINOP_MUL: compile_binop(wc, inst, cmd_assign, VAR_OP_TIMES); break;
        case WIR_BINOP_DIV: compile_binop(wc, inst, cmd_assign, VAR_OP_DIV); break;
        case WIR_BINOP_MOD: compile_binop(wc, inst, cmd_assign, VAR_OP_MOD); break;
        case WIR_BINOP_XOR: compile_binop(wc, inst, cmd_assign, VAR_OP_XOR); break;
        case WIR_BINOP_LSH: compile_binop(wc, inst, cmd_assign, VAR_OP_LSHIFT); break;
        case WIR_BINOP_AND: compile_binop(wc, inst, cmd_assign, VAR_OP_AND); break;
        case WIR_BINOP_OR:  compile_binop(wc, inst, cmd_assign, VAR_OP_OR); break;
        }
        break;                
    }
    case _WIRInst_Compare: {
        WIRInst_Compare *inst = (WIRInst_Compare *)wi;
        switch (inst->op) {
            case WIR_CMP_EQ:   compile_compare(wc, inst, IF_INT_OP_EQ); break;
            case WIR_CMP_NEQ:  compile_compare(wc, inst, IF_INT_OP_NEQ); break;
            case WIR_CMP_LT:   compile_compare(wc, inst, IF_INT_OP_LT); break;
            case WIR_CMP_LTE:  compile_compare(wc, inst, IF_INT_OP_LTE); break;
            case WIR_CMP_GT:   compile_compare(wc, inst, IF_INT_OP_GT); break;
            case WIR_CMP_GTE:  compile_compare(wc, inst, IF_INT_OP_GTE); break;
        }
        break;
    }
    case _WIRInst_StrAssign: {
        WIRInst_StrAssign *inst = (WIRInst_StrAssign *)wi;
        push_str_command(wc, resolve(wc, inst->dest), inst->src);
        break;
    }
    case _WIRInst_IfBegin: {
        WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wi;
        
        int flag = 0;
        if (find_else(wc->wcev, index))
            flag = 0x10;

        {
            VEC_int32_t int_fields = VEC_EMPTY;
            VEC_PUSH(int_fields, 1 | flag, wc->arena);
            VEC_PUSH(int_fields, resolve(wc, inst->cond), wc->arena);
            VEC_PUSH(int_fields, 0, wc->arena);
            VEC_PUSH(int_fields, IF_INT_OP_NEQ, wc->arena);
    
            cev_push_cmd(wc->cev, CMD_IF_INT, wc->indent,
                int_fields, (VEC_StringView)VEC_EMPTY);
        }

        {
            VEC_int32_t int_fields = VEC_EMPTY;
            VEC_PUSH(int_fields, 1, wc->arena);
    
            cev_push_cmd(wc->cev, CMD_BRANCH, wc->indent,
                int_fields, (VEC_StringView)VEC_EMPTY);
        }
        
        wc->indent++;
        break;
    }
    case _WIRInst_IfBeginOp: {
        WIRInst_IfBeginOp *inst = (WIRInst_IfBeginOp *)wi;
        
        int flag = 0;
        if (find_else(wc->wcev, index))
            flag = 0x10;

        {
            VEC_int32_t int_fields = VEC_EMPTY;
            VEC_PUSH(int_fields, 1 | flag, wc->arena);
            VEC_PUSH(int_fields, resolve(wc, inst->a), wc->arena);
            VEC_PUSH(int_fields, resolve(wc, inst->b), wc->arena);

            switch (inst->op) {
            case WIR_CMP_EQ:
                VEC_PUSH(int_fields, IF_INT_OP_EQ, wc->arena); break;
            case WIR_CMP_NEQ:
                VEC_PUSH(int_fields, IF_INT_OP_NEQ, wc->arena); break;
            case WIR_CMP_LT:
                VEC_PUSH(int_fields, IF_INT_OP_LT, wc->arena); break;
            case WIR_CMP_LTE:
                VEC_PUSH(int_fields, IF_INT_OP_LTE, wc->arena); break;
            case WIR_CMP_GT:
                VEC_PUSH(int_fields, IF_INT_OP_GT, wc->arena); break;
            case WIR_CMP_GTE:
                VEC_PUSH(int_fields, IF_INT_OP_GTE, wc->arena); break;
            }
    
            cev_push_cmd(wc->cev, CMD_IF_INT, wc->indent,
                int_fields, (VEC_StringView)VEC_EMPTY);
        }

        {
            VEC_int32_t int_fields = VEC_EMPTY;
            VEC_PUSH(int_fields, 1, wc->arena);
    
            cev_push_cmd(wc->cev, CMD_BRANCH, wc->indent,
                int_fields, (VEC_StringView)VEC_EMPTY);
        }
        
        wc->indent++;
        break;
    }
    case _WIRInst_Else: {
        VEC_int32_t int_fields = VEC_EMPTY;
        VEC_PUSH(int_fields, 0, wc->arena);

        cev_push_cmd(wc->cev, CMD_BRANCH_ELSE, wc->indent - 1,
            int_fields, (VEC_StringView)VEC_EMPTY);
        break;
    }
    case _WIRInst_IfEnd: {
        wc->indent--;
        cev_push_simple_cmd(wc->cev, CMD_IF_END, wc->indent);
        break;
    }
    case _WIRInst_LoopBegin: {
        cev_push_simple_cmd(wc->cev, CMD_LOOP, wc->indent);
        
        wc->indent++;
        break;
    }
    case _WIRInst_LoopBeginN: {
        WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wi;

        VEC_int32_t i_vec = VEC_EMPTY;
        int32_t n = resolve(wc, inst->count);
        
        VEC_PUSH(i_vec, n, wc->arena);
        
        cev_push_cmd(wc->cev, CMD_LOOP_COUNT, wc->indent,
            i_vec, (VEC_StringView)VEC_EMPTY);
        
        wc->indent++;
        break;
    }
    case _WIRInst_LoopEnd: {
        wc->indent--;
        cev_push_simple_cmd(wc->cev, CMD_LOOP_END, wc->indent);
        break;
    }
    case _WIRInst_Continue: {
        cev_push_simple_cmd(wc->cev, CMD_CONTINUE, wc->indent);
        break;
    }
    case _WIRInst_Break: {
        cev_push_simple_cmd(wc->cev, CMD_BREAK, wc->indent);
        break;
    }
    case _WIRInst_Call: {
        WIRInst_Call *inst = (WIRInst_Call *)wi;

        int32_t cev = resolve(wc, inst->cev);

        VEC_int32_t int_args = VEC_EMPTY;
        VEC_int32_t str_ref_args = VEC_EMPTY;
        VEC_StringView str_lit_args = VEC_EMPTY;
        VEC_PUSH(str_lit_args, SV(""), wc->arena);

        int32_t total_int_args = 0;
        int32_t total_str_args = 0;
        int32_t strlit_flags = 0;
        for (size_t i = 0; i < inst->args.count; i++) {
            WIROperand wop = inst->args.at[i];
            if (op_is_string(wop)) {
                if (op_is_strlit(wop)) {
                    VEC_PUSH(str_ref_args, 0, wc->arena);
                    VEC_PUSH(str_lit_args, interpolate(wc, wop), wc->arena);
                    strlit_flags |= (1 << total_str_args);
                } else {
                    VEC_PUSH(str_ref_args, resolve(wc, wop), wc->arena);
                    VEC_PUSH(str_lit_args, SV(""), wc->arena);
                }
                total_str_args++;
            } else {
                VEC_PUSH(int_args, resolve(wc, wop), wc->arena);
                total_int_args++;
            }
        }

        // TODO: This can be expanded later.
        assert(total_int_args <= 5 && total_str_args <= 5);

        int32_t flags =
            total_int_args
            | total_str_args << 4
            | strlit_flags << 12;

        // Handle storing the result.
        if (!(inst->dest.kind == OPKIND_IMM_INT
            && inst->dest.as.imm_int == 0)) {

            flags |= CALL_STORES_RETURN;
        }

        VEC_int32_t int_fields = VEC_EMPTY;
        VEC_PUSH(int_fields, cev, wc->arena);
        VEC_PUSH(int_fields, flags, wc->arena);

        for (size_t i = 0; i < int_args.count; i++)
            VEC_PUSH(int_fields, int_args.at[i], wc->arena);
        
        for (size_t i = 0; i < str_ref_args.count; i++)
            VEC_PUSH(int_fields, str_ref_args.at[i], wc->arena);

        if (flags & CALL_STORES_RETURN)
            VEC_PUSH(int_fields, resolve(wc, inst->dest), wc->arena);

        cev_push_cmd(wc->cev,
            CMD_CALL_ID, wc->indent,
            int_fields,
            strlit_flags ? str_lit_args : (VEC_StringView)VEC_EMPTY
        );
        break;
    }
    case _WIRInst_DBLoad: {
        WIRInst_DBLoad *inst = (WIRInst_DBLoad *)wi;
        VEC_int32_t int_fields = VEC_EMPTY;
        VEC_PUSH(int_fields, resolve(wc, inst->db_type), wc->arena);
        VEC_PUSH(int_fields, resolve(wc, inst->db_data), wc->arena);
        VEC_PUSH(int_fields, resolve(wc, inst->db_field), wc->arena);

        int32_t flags = DB_LOAD;
        switch (inst->assign) {
            case WIR_ASSIGN_EQ:  flags |= DB_ASSIGN_EQ; break;
            case WIR_ASSIGN_ADD: flags |= DB_ASSIGN_PLUS_EQ; break;
            case WIR_ASSIGN_SUB: flags |= DB_ASSIGN_MINUS_EQ; break;
            case WIR_ASSIGN_MUL: flags |= DB_ASSIGN_TIMES_EQ; break;
            case WIR_ASSIGN_DIV: flags |= DB_ASSIGN_DIV_EQ; break;
            case WIR_ASSIGN_MOD: flags |= DB_ASSIGN_MOD_EQ; break;
        }
        switch (inst->db_kind) {
            case DB_UDB: flags |= DB_KIND_UDB; break;
            case DB_CDB: flags |= DB_KIND_CDB; break;
        }
        VEC_PUSH(int_fields, flags, wc->arena);
        VEC_PUSH(int_fields, resolve(wc, inst->dst), wc->arena);

        VEC_StringView str_fields = VEC_EMPTY;
        VEC_PUSH(str_fields, SV(""), wc->arena);
        VEC_PUSH(str_fields, SV(""), wc->arena);
        VEC_PUSH(str_fields, SV(""), wc->arena);
        VEC_PUSH(str_fields, SV(""), wc->arena);

        cev_push_cmd(wc->cev, CMD_DB, wc->indent, int_fields, str_fields);
        break;
    }
    case _WIRInst_DBStore: {
        WIRInst_DBStore *inst = (WIRInst_DBStore *)wi;
        VEC_int32_t int_fields = VEC_EMPTY;
        VEC_PUSH(int_fields, resolve(wc, inst->db_type), wc->arena);
        VEC_PUSH(int_fields, resolve(wc, inst->db_data), wc->arena);
        VEC_PUSH(int_fields, resolve(wc, inst->db_field), wc->arena);

        int32_t flags = DB_STORE;
        if (op_is_strlit(inst->src)) flags |= DB_SRCDST_STRLIT;
        switch (inst->assign) {
            case WIR_ASSIGN_EQ:  flags |= DB_ASSIGN_EQ; break;
            case WIR_ASSIGN_ADD: flags |= DB_ASSIGN_PLUS_EQ; break;
            case WIR_ASSIGN_SUB: flags |= DB_ASSIGN_MINUS_EQ; break;
            case WIR_ASSIGN_MUL: flags |= DB_ASSIGN_TIMES_EQ; break;
            case WIR_ASSIGN_DIV: flags |= DB_ASSIGN_DIV_EQ; break;
            case WIR_ASSIGN_MOD: flags |= DB_ASSIGN_MOD_EQ; break;
        }
        switch (inst->db_kind) {
            case DB_UDB: flags |= DB_KIND_UDB; break;
            case DB_CDB: flags |= DB_KIND_CDB; break;
        }
        VEC_PUSH(int_fields, flags, wc->arena);
        if (!(flags & DB_SRCDST_STRLIT))
            VEC_PUSH(int_fields, resolve(wc, inst->src), wc->arena);

        VEC_StringView str_fields = VEC_EMPTY;
        VEC_PUSH(str_fields, flags & DB_SRCDST_STRLIT ?
            interpolate(wc, inst->src) : SV(""), wc->arena);
        VEC_PUSH(str_fields, SV(""), wc->arena);
        VEC_PUSH(str_fields, SV(""), wc->arena);
        VEC_PUSH(str_fields, SV(""), wc->arena);

        cev_push_cmd(wc->cev, CMD_DB, wc->indent, int_fields, str_fields);
        break;
    }
    case _WIRInst_Cmd: {
        WIRInst_Cmd *inst = (WIRInst_Cmd *)wi;
        assert(inst->open_close >= -1
                && inst->open_close <= 1);

        VEC_int32_t int_fields = VEC_EMPTY;
        for (size_t i = 0; i < inst->iargs.count; i++) {
            VEC_PUSH(int_fields, resolve(wc, inst->iargs.at[i]), wc->arena);
        }

        VEC_StringView str_fields = VEC_EMPTY;
        for (size_t i = 0; i < inst->sargs.count; i++) {
            WIROperand wop = inst->sargs.at[i];
            if (op_is_strlit(wop))
                VEC_PUSH(str_fields, interpolate(wc, wop), wc->arena);
            else UNREACHABLE;
        }

        if (inst->open_close == -1)
            wc->indent--;

        cev_push_cmd(wc->cev,
            inst->op,
            wc->indent,
            int_fields, str_fields);

        if (inst->open_close == 1)
            wc->indent++;
        
        break;
    }
    case _WIRInst_ReturnVal: {
        WIRInst_ReturnVal *inst = (WIRInst_ReturnVal *)wi;

        // TODO: Optimize by not moving values if they are already in
        //       the correct index to return.
        //
        //       This is a little awkward because of all the possible cases:
        //       If a value is already stored in the right CSelf, then
        //       don't move; but if a value is stored in a global (because
        //       for example it overflowed the CSelf space), then move.
        //       If a value is an immediate, it has to be stored first.
        //       And so on.

        int32_t *target = &wc->cev->RETURN_VAL_TARGET;

        switch (inst->val.kind) {
        case OPKIND_IMM_INT:
        case OPKIND_LOCAL_INT:
        case OPKIND_TEMP_INT:
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_CEV:
            *target = 99;
            push_binop_command(wc, *target + CSELF_BASE,
                VAR_ASSIGN_EQ, resolve(wc, inst->val), 0, VAR_OP_PLUS);
            break;
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_LOCAL_STR:
        case OPKIND_TEMP_STR:
        case OPKIND_GLOBAL_STR:
            *target = 9;
            push_str_command(wc, 
                *target + CSELF_BASE, inst->val);
            break;
        case OPKIND_GLOBAL_UDBTYPE:
        case OPKIND_GLOBAL_CDBTYPE:
        case OPKIND_DBDATA:
        case OPKIND_DBFIELD:
            UNREACHABLE;
        }

        cev_push_simple_cmd(wc->cev, CMD_RETURN, wc->indent);
        break;
    }
    case _WIRInst_ReturnVoid: {
        cev_push_simple_cmd(wc->cev, CMD_RETURN, wc->indent);
        break;
    }
    }
}

// If the temporary result of a calculation is moved directly
// into a variable, rewrite to store the result directly into the variable.
static void temp_copy_propagation_pass(WIRCev *wcev) {
    // Start from index 1, since this can't happen if there is no
    // preceding instruction.
    for (size_t i = 1; i < wcev->insts.count; i++) {
        if (wcev->insts.at[i]->kind != _WIRInst_Binop)
            continue;
        
        WIRInst_Binop *inst = (WIRInst_Binop *)wcev->insts.at[i]; 

        if (!(inst->op == WIR_BINOP_ADD
              && inst->a.kind == OPKIND_TEMP_INT
              && inst->b.kind == OPKIND_IMM_INT
              && inst->b.as.imm_int == 0))
            continue;

        WIRInst *prev = wcev->insts.at[i - 1];

        if (prev->kind == _WIRInst_Binop) {
            WIRInst_Binop *p = (WIRInst_Binop *)prev;
            if (p->dest.kind == OPKIND_TEMP_INT
                && p->assign == WIR_ASSIGN_EQ
                && p->dest.as.offset == inst->a.as.offset) {

                p->dest = inst->dest;
                p->assign = inst->assign;
                inst->base.kind = _WIRInst_NOP;
            }
        } else if (prev->kind == _WIRInst_Compare) {
            WIRInst_Compare *p = (WIRInst_Compare *)prev;
            if (p->dest.kind == OPKIND_TEMP_INT
                && p->dest.as.offset == inst->a.as.offset) {

                p->dest = inst->dest;
                inst->base.kind = _WIRInst_NOP;
            }
        } else if (prev->kind == _WIRInst_Call) {
            WIRInst_Call *p = (WIRInst_Call *)prev;
            if (p->dest.kind == OPKIND_TEMP_INT
                && p->dest.as.offset == inst->a.as.offset) {

                p->dest = inst->dest;
                inst->base.kind = _WIRInst_NOP;
            }
        } else if (prev->kind == _WIRInst_DBLoad) {
            WIRInst_DBLoad *p = (WIRInst_DBLoad *)prev;
            if (p->dst.kind == OPKIND_TEMP_INT
                && p->dst.as.offset == inst->a.as.offset) {

                p->dst = inst->dest;
                inst->base.kind = _WIRInst_NOP;
            }
        }
    }
}

// If the temporary result of a comparison operation is immediately negated with
// an XOR instruction, then merge the two instructions into one that just uses the
// negated comparison.
static void comp_reverse_pass(WIRCev *wcev) {
    // Start from index 1, since this can't happen if there is no
    // preceding instruction.
    for (size_t i = 1; i < wcev->insts.count; i++) {
        if (wcev->insts.at[i]->kind != _WIRInst_Binop)
            continue;
        
        WIRInst_Binop *inst = (WIRInst_Binop *)wcev->insts.at[i]; 

        if (!(inst->op == WIR_BINOP_XOR
              && inst->assign == WIR_ASSIGN_EQ
              && inst->a.kind == OPKIND_TEMP_INT
              && inst->b.kind == OPKIND_IMM_INT
              && inst->b.as.imm_int == 1))
            continue;

        WIRInst *prev = wcev->insts.at[i - 1];

        if (prev->kind != _WIRInst_Compare) continue;

        WIRInst_Compare *p = (WIRInst_Compare *)prev;
        if (p->dest.kind == OPKIND_TEMP_INT
            && p->dest.as.offset == inst->a.as.offset) {

            p->dest = inst->dest;
            switch (p->op) {
                case WIR_CMP_EQ:  p->op = WIR_CMP_NEQ; break;
                case WIR_CMP_NEQ: p->op = WIR_CMP_EQ; break;
                case WIR_CMP_LT:  p->op = WIR_CMP_GTE; break;
                case WIR_CMP_LTE: p->op = WIR_CMP_GT; break;
                case WIR_CMP_GT:  p->op = WIR_CMP_LTE; break;
                case WIR_CMP_GTE: p->op = WIR_CMP_LT; break;
            }
            
            inst->base.kind = _WIRInst_NOP;

            // Swap the binop and compare so that it's easier to chain
            // into the `ifop` optimization (otherwise the NOPs would
            // block the way).
            wcev->insts.at[i - 1] = (WIRInst *)inst;
            wcev->insts.at[i] = prev;
        }
    }
}

// If a raw `if` instruction immediately follows a compare instruction
// and uses the compare's temporary result, then replace the `if`
// with a more efficient `ifop`.
//
// Similarly, if a raw `if` immediately follows the negation of a
// temporary, merge the negation into an `ifop`.
static void comp_if_pass(WIRCev *wcev, Arena *arena) {
    // Start from index 1, since this can't happen if there is no
    // preceding instruction.
    for (size_t i = 1; i < wcev->insts.count; i++) {
        if (wcev->insts.at[i]->kind != _WIRInst_IfBegin)
            continue;
        
        WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wcev->insts.at[i]; 
        if (inst->cond.kind != OPKIND_TEMP_INT) continue;
        
        WIRInst *prev = wcev->insts.at[i - 1];
        if (prev->kind == _WIRInst_Compare) {
            WIRInst_Compare *p = (WIRInst_Compare *)prev;
                
            // If the if condition is the temporary we just
            // handled, then the `if` instruction can be upgraded
            // to an `ifop`.
            if (p->dest.kind == OPKIND_TEMP_INT
                && inst->cond.as.offset == p->dest.as.offset) {
    
                WIRInst_IfBeginOp *ifop = 
                    arena_alloc_assert(arena, sizeof(WIRInst_IfBeginOp));
            
                *ifop = (WIRInst_IfBeginOp){
                    .base.kind = _WIRInst_IfBeginOp,
                    .a = p->a,
                    .b = p->b,
                    .op = p->op
                };
    
                prev->kind = _WIRInst_NOP;
                wcev->insts.at[i] = (WIRInst *)ifop;
            }
        } else if (prev->kind == _WIRInst_Binop) {
            WIRInst_Binop *p = (WIRInst_Binop *)prev;

            // If the if condition is a negated temporary,
            // then just negate the if condition and remove the
            // negation instruction.
            if (p->dest.kind == OPKIND_TEMP_INT
                && inst->cond.as.offset == p->dest.as.offset
                && p->op == WIR_BINOP_XOR
                && p->assign == WIR_ASSIGN_EQ
                && p->a.kind == OPKIND_TEMP_INT
                && p->b.kind == OPKIND_IMM_INT
                && p->b.as.imm_int == 1) {
    
                WIRInst_IfBeginOp *ifop = 
                    arena_alloc_assert(arena, sizeof(WIRInst_IfBeginOp));
            
                *ifop = (WIRInst_IfBeginOp){
                    .base.kind = _WIRInst_IfBeginOp,
                    .a = p->a,
                    .b = WIR_IMM_I(0),
                    .op = WIR_CMP_EQ
                };
    
                prev->kind = _WIRInst_NOP;
                wcev->insts.at[i] = (WIRInst *)ifop;
            }
        }
    }
}

static void compile_dbs(WIRCompiler *wc, VEC_WIRDB *g, VEC_DBType *dbs) {
    for (size_t i = 0; i < g->count; i++) {
        WIRDB *wdb = &g->at[i];
        
        DBType db;
        db_init(&db);
        db.TYPENAME = wdb->qualifier.name;

        // Handle fields.
        for (size_t j = 0; j < wdb->fields.count; j++) {
            WIRField *field = &wdb->fields.at[j];
            VEC_PUSH(db.itemdef, ((DBItemDef){
                .type = field->type == WIRFIELD_INT ?
                    DBITEM_INT : DBITEM_STR,
                .ITEMNAME = field->name,
                .VMEMO = SV(""),
                .CHOICE_NAME = VEC_EMPTY,
                .CHOICE_VAL = VEC_EMPTY,
                .DEFAULT_VAL = field->has_initializer ?
                    resolve(wc, field->initializer) : 0 
            }), wc->arena);
        }

        // Handle data.
        for (size_t j = 0; j < wdb->data.count; j++) {
            VEC_DBVal values = VEC_EMPTY;
            WIRData wdata = wdb->data.at[j];
            for (size_t k = 0; k < wdata.values.count; k++) {
                WIROperand wop = wdata.values.at[k];
                if (op_is_strlit(wop))
                    VEC_PUSH(values, (DBVal){ .str_val = interpolate(wc, wop) }, wc->arena);
                else 
                    VEC_PUSH(values, (DBVal){ .int_val = resolve(wc, wop) }, wc->arena);
            }
            VEC_PUSH(db.data, ((DBData){ .name = wdata.name, .values = values }), wc->arena);
        }

        VEC_PUSH(*dbs, db, wc->arena);
    }
}

static void compile_wir(WIRCompiler *wc) {
    WIR *wir = wc->wir;

    // Process global variables.
    for (size_t i = 0; i < wir->g_ints.count; i++)
        push_sdb_int(wc, wir->g_ints.at[i].name);
    for (size_t i = 0; i < wir->g_strs.count; i++)
        push_sdb_str(wc, wir->g_strs.at[i].name);

    // Process every DB.
    compile_dbs(wc, &wir->g_udbs, &wc->gd.udb);
    compile_dbs(wc, &wir->g_cdbs, &wc->gd.cdb);
    
    // Process every `WIRCev`.
    for (size_t i = 0; i < wir->g_cevs.count; i++) {
        WIRCev *wcev = &wir->g_cevs.at[i];
        wc->panic_mode = false;

        // First, apply transformations to the code.
        disable_rc_pass(wc->arena, wcev);
        temp_copy_propagation_pass(wcev);
        comp_reverse_pass(wcev);
        comp_if_pass(wcev, wc->arena);
        
        // Map concrete addresses to temporaries.
        addr_alloc_pass(wc, wcev);
    }

    // Then compile the code into commands.
    // This has to be done in a separate pass as common events may need
    // context about other common events' address maps that isn't computed
    // until after the first pass. 
    for (size_t i = 0; i < wir->g_cevs.count; i++) {
        wc->wcev = &wir->g_cevs.at[i];

        CommonEvent cev;
        cev_init(&cev, wc->arena);
        cev.COMMON_NAME = wc->wcev->qualifier.name;
        wc->cev = &cev;
        wc->indent = 0;

        for (size_t j = 0; j < wc->wcev->insts.count; j++) {
            compile_inst(wc, j);
        }
        assert(wc->indent == 0);
    
        cev_push_cmd(&cev, 0, wc->indent,
            (VEC_int32_t)VEC_EMPTY, (VEC_StringView)VEC_EMPTY);

        VEC_PUSH(wc->gd.cevs, cev, wc->arena);
    }
}

// Ensures that the IR is sane. That is:
// - Local variables are not referenced unless they are pushed onto 
//   the virtual stack.
// - Loops and conditionals have a matching beginning and end.
// - Convenience properties of the IR cached in the struct actually
//   properly reflect the properties of the IR. (i.e. The "next unused
//   temporaries" should actually be unused.)
// - Instructions that store a value (like binop or call) do not attempt
//   to store values into immediates (which have no addresses).
// It's expected that any WIR generated by AST2WIR has these properties,
// so the compiler will just crash if those requirements are not met.
static bool validate(WIRCompiler *wc, WIRCev *wcev) {
    // TODO: Basically this is a massive assertion which wouldn't
    //       actually change the output so just holding off on this for now.
    (void) wc;
    (void) wcev;
    return true;
}

GameData wir_pass(WIR *wir, Arena *arena) {
    WIRCompiler wc = {
        .wir = wir,
        .arena = arena,
        .had_error = false,
        .exaddr_int_base = 0,
        .exaddr_str_base = 0,
    };
    gd_init(&wc.gd);

    // Using SDB from version 3.713.
    wc.gd.sdb = sdb_3713(arena);

    // By default, the variable-related SDB types already have a few empty elements.
    // Clear them out for simplicity.
    wc.gd.sdb.at[SDB_NORMAL_VAR_DBTYPE].data.count = 0;
    wc.gd.sdb.at[SDB_STRING_VAR_DBTYPE].data.count = 0;
    
    compile_wir(&wc);

    // Assign entry point.
    for (size_t i = 0; i < wir->g_cevs.count; i++) {
        if (sv_equals(wir->g_cevs.at[i].qualifier.name, SV("main"))) {
            wc.gd.entry = 500000 + i;
        }
    }

    // The editor crashes if there are no DBs of a type,
    // so if there are none then add an empty one.
    if (wc.gd.udb.count == 0) {
        DBType db; db_init(&db);
        VEC_PUSH(wc.gd.udb, db, arena);
    }
    if (wc.gd.cdb.count == 0) {
        DBType db; db_init(&db);
        VEC_PUSH(wc.gd.cdb, db, arena);
    }

    return wc.gd;
}

void wir_init(WIR *wir) {
    VEC_INIT(wir->g_ints);
    VEC_INIT(wir->g_strs);
    VEC_INIT(wir->g_cevs);
    VEC_INIT(wir->g_udbs);
    VEC_INIT(wir->g_cdbs);
}

static void print_wop(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_INT:
            printf(" %d", wop.as.imm_int);
            return;
        case OPKIND_IMM_STR:
            printf(" \"" SV_FMT "\"", SV_FMT_VAL(wop.as.imm_str));
            return;
        case OPKIND_INTERP:
            printf(" ${");
            for (size_t i = 0; i < wop.as.interp.count; i++) {
                print_wop(wop.as.interp.at[i]);
            }
            printf(" }");
            return;
        case OPKIND_LOCAL_INT:
            printf(" $LI(%zu)", wop.as.offset);
            return;
        case OPKIND_LOCAL_STR:
            printf(" $LS(%zu)", wop.as.offset);
            return;
        case OPKIND_TEMP_INT:
            printf(" $TI(%zu)", wop.as.offset);
            return;
        case OPKIND_TEMP_STR:
            printf(" $TS(%zu)", wop.as.offset);
            return;
        case OPKIND_GLOBAL_INT:
            printf(" $GINT[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_STR:
            printf(" $GSTR[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_CEV:
            printf(" $GCEV[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_UDBTYPE:
            printf(" $GUDB[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_CDBTYPE:
            printf(" $GCDB[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_DBDATA:
        case OPKIND_DBFIELD:
            UNREACHABLE;
            return;
    }
}

void print_wir(WIR *wir) {
    for (size_t cev = 0; cev < wir->g_cevs.count; cev++) {
        VEC_PTR_WIRInst arr = wir->g_cevs.at[cev].insts;

        size_t stack_i = 0;
        size_t stack_s = 0;

        printf(SV_FMT ":\n", SV_FMT_VAL(wir->g_cevs.at[cev].qualifier.name));

        for (size_t i = 0; i < arr.count; i++) {
            WIRInst *inst = arr.at[i];
            switch (inst->kind) {
            case _WIRInst_PushIntN: {
                WIRInst_PushIntN *in = (WIRInst_PushIntN *)inst;
                stack_i += in->n;
                printf("pushi %zu \t\t\t; (max i: %zu)", in->n, stack_i-1);
                break;
            }
            case _WIRInst_PushStrN: {
                WIRInst_PushStrN *in = (WIRInst_PushStrN *)inst;
                stack_s += in->n;
                printf("pushs %zu \t\t\t; (max s: %zu)", in->n, stack_s-1);
                break;
            }
            case _WIRInst_PopIntN: {
                WIRInst_PopIntN *in = (WIRInst_PopIntN *)inst;
                stack_i -= in->n;
                printf("popi %zu \t\t\t", in->n);
                if (stack_i == 0)
                    printf("; (max i: -)");
                else
                    printf("; (max i: %zu)", stack_i - 1);
                break;
            }
            case _WIRInst_PopStrN: {
                WIRInst_PopStrN *in = (WIRInst_PopStrN *)inst;
                stack_s -= in->n;
                printf("pops %zu \t\t\t", in->n);
                if (stack_s == 0)
                    printf("; (max s: -)");
                else
                    printf("; (max s: %zu)", stack_s - 1);
                break;
            }
            case _WIRInst_Binop: {
                WIRInst_Binop *in = (WIRInst_Binop *)inst;
                printf("binop");
                print_wop(in->dest);
                printf(" %d", in->assign);
                print_wop(in->a);
                printf(" %d", in->op);
                print_wop(in->b);
                break;
            }
            case _WIRInst_Compare: {
                WIRInst_Compare *in = (WIRInst_Compare *)inst;
                printf("cmp");
                print_wop(in->dest);
                print_wop(in->a);
                printf(" %d", in->op);
                print_wop(in->b);
                break;
            }
            case _WIRInst_StrAssign: {
                WIRInst_StrAssign *in = (WIRInst_StrAssign *)inst;
                printf("str");
                print_wop(in->dest);
                print_wop(in->src);
                break;
            }
            case _WIRInst_ReturnVal: {
                WIRInst_ReturnVal *in = (WIRInst_ReturnVal *)inst;
                printf("retv");
                print_wop(in->val);
                break;
            }
            case _WIRInst_ReturnVoid: {
                printf("ret");
                break;
            }
            case _WIRInst_Call: {
                WIRInst_Call *in = (WIRInst_Call *)inst;
                printf("call");
                print_wop(in->dest);
                print_wop(in->cev);
                for (size_t arg = 0; arg < in->args.count; arg++) {
                    print_wop(in->args.at[arg]);
                }
                break;
            }
            case _WIRInst_Cmd: {
                WIRInst_Cmd *in = (WIRInst_Cmd *)inst;
                printf("cmd %d %d", in->op, in->open_close);
                for (size_t arg = 0; arg < in->iargs.count; arg++)
                    print_wop(in->iargs.at[arg]);
                for (size_t arg = 0; arg < in->sargs.count; arg++)
                    print_wop(in->sargs.at[arg]);
                break;
            }
            case _WIRInst_DBLoad: {
                WIRInst_DBLoad *in = (WIRInst_DBLoad *)inst;
                printf("load %d", in->db_kind);
                print_wop(in->dst);
                printf(" %d", in->assign);
                print_wop(in->db_type);
                print_wop(in->db_data);
                print_wop(in->db_field);
                break;
            }
            case _WIRInst_DBStore: {
                WIRInst_DBStore *in = (WIRInst_DBStore *)inst;
                printf("store %d", in->db_kind);
                print_wop(in->db_type);
                print_wop(in->db_data);
                print_wop(in->db_field);
                printf(" %d", in->assign);
                print_wop(in->src);
                break;
            }
            case _WIRInst_LoopBegin: {
                printf("loop");
                break;
            }
            case _WIRInst_LoopBeginN: {
                WIRInst_LoopBeginN *in = (WIRInst_LoopBeginN *)inst;
                printf("loopn");
                print_wop(in->count);
                break;
            }
            case _WIRInst_LoopEnd: {
                printf("endloop");
                break;
            }
            case _WIRInst_IfBegin: {
                WIRInst_IfBegin *in = (WIRInst_IfBegin *)inst;
                printf("if");
                print_wop(in->cond);
                break;
            }
            case _WIRInst_IfBeginOp: {
                WIRInst_IfBeginOp *in = (WIRInst_IfBeginOp *)inst;
                printf("ifop");
                print_wop(in->a);
                printf(" %d", in->op);
                print_wop(in->b);
                break;
            }
            case _WIRInst_Else: {
                printf("else");
                break;
            }
            case _WIRInst_IfEnd: {
                printf("endif");
                break;
            }
            case _WIRInst_Continue: {
                printf("continue");
                break;
            }
            case _WIRInst_Break: {
                printf("break");
                break;
            }
            case _WIRInst_NOP:
                printf("nop");
                break;
            default:
                printf("(op %d)", inst->kind);
                printf(" (print not implemented)");
                break;
            }
            printf("\n");
        }
        printf("\n");
    }

    for (size_t i = 0; i < wir->g_udbs.count; i++) {
        WIRDB *db = &wir->g_udbs.at[i];

        printf("(udb type) " SV_FMT ":\n", SV_FMT_VAL(db->qualifier.name));
        for (size_t j = 0; j < db->fields.count; j++) {
            WIRField *field = &db->fields.at[j];
            
            printf(SV_FMT "\n", SV_FMT_VAL(field->name));
        }
    }

    for (size_t i = 0; i < wir->g_cdbs.count; i++) {
        WIRDB *db = &wir->g_cdbs.at[i];

        printf("(cdb type) " SV_FMT ":\n", SV_FMT_VAL(db->qualifier.name));
        for (size_t j = 0; j < db->fields.count; j++) {
            WIRField *field = &db->fields.at[j];
            
            printf(SV_FMT "\n", SV_FMT_VAL(field->name));
        }
    }
}

bool op_is_local(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_INT:
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_TEMP_INT:
        case OPKIND_TEMP_STR:
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_STR:
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDBTYPE:
        case OPKIND_GLOBAL_CDBTYPE:
            return false;
        case OPKIND_LOCAL_INT:
        case OPKIND_LOCAL_STR:
            return true;
        case OPKIND_DBDATA:
        case OPKIND_DBFIELD:
            UNREACHABLE;
            return false;
    }
    UNREACHABLE;
    return false;
}

bool op_is_global(WIROperand wop) {
    switch (wop.kind) {
        case OPKIND_IMM_INT:
        case OPKIND_IMM_STR:
        case OPKIND_INTERP:
        case OPKIND_TEMP_INT:
        case OPKIND_TEMP_STR:
        case OPKIND_LOCAL_INT:
        case OPKIND_LOCAL_STR:
            return false;
        case OPKIND_GLOBAL_INT:
        case OPKIND_GLOBAL_STR:
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDBTYPE:
        case OPKIND_GLOBAL_CDBTYPE:
            return true;
        case OPKIND_DBDATA:
        case OPKIND_DBFIELD:
            UNREACHABLE;
            return false;
    }
    UNREACHABLE;
    return false;
}