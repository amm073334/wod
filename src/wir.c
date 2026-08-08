#include "wir.h"

#define CSELF_BASE 1600000
#define CSELF_INT_BASE 1600010
#define CSELF_INT_MAX  1600099
#define CSELF_STR_BASE 1600005
#define CSELF_STR_MAX  1600009

#define RC_THRESHOLD 1000000
#define NORMAL_VAR_BASE 2000000
#define STRING_VAR_BASE 3000000
#define CEV_BASE 500000
#define UDB_BASE 10000000
#define CDB_BASE 11000000

typedef struct GlobalEntry {
    StringView path;
    StringView name;
} GlobalEntry;
VEC_DEF(GlobalEntry);

typedef struct WIRCompiler {
    // Maps globally-qualified names to global indices.
    VEC_GlobalEntry g_cevs;
    VEC_GlobalEntry g_cdbs;
    VEC_GlobalEntry g_udbs;
    VEC_GlobalEntry g_ints;
    VEC_GlobalEntry g_strs;

    // Maps temporaries to concrete references.
    VEC_int32_t int_map;
    VEC_int32_t str_map;

    // Information about the current common event.
    CommonEvent *cev;
    uint8_t indent;

    GameData gd;
    Arena *arena;
} WIRCompiler;

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
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
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
        .op = WIR_SUB,
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
        case _WIRInst_TOMBSTONE:
        case _WIRInst_PushInt:
        case _WIRInst_PushStr:
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
        case _WIRInst_IfBegin: {
            WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wirinst;
            disable_rc(arena, wcev, &i, &inst->cond);
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

// Resolves a non-string `WIROperand` into a concrete integer value.
// Assumes reference conversion has already been disabled.
static int32_t resolve(WIRCompiler *wc, WIROperand wop) {
    assert(wop.kind != OPKIND_IMM_STR && wop.kind != OPKIND_INTERP);
    
    VEC_GlobalEntry *g_vec = NULL;
    size_t offset = 0;

    switch (wop.kind) {
    case OPKIND_IMM_INT: {
        return wop.as.imm_int;
    }
    // For the time being, don't allow addresses to escape the CSelf space.
    case OPKIND_LOCAL_INT: {
        int32_t ref = wop.as.offset + CSELF_INT_BASE;
        assert(ref <= CSELF_INT_MAX);
        return ref;
    }
    case OPKIND_LOCAL_STR: {
        int32_t ref = wop.as.offset + CSELF_STR_BASE;
        assert(ref <= CSELF_STR_MAX);
        return ref;
    }
    case OPKIND_TEMP_INT: {
        int32_t ref = wc->int_map.at[wop.as.offset];
        assert(ref >= RC_THRESHOLD);
        assert(ref <= CSELF_INT_MAX);
        return ref;
    }
    case OPKIND_TEMP_STR: {
        int32_t ref = wc->str_map.at[wop.as.offset];
        assert(ref >= RC_THRESHOLD);
        assert(ref <= CSELF_STR_MAX);
        return ref;
    }
    case OPKIND_GLOBAL_INT: g_vec = &wc->g_ints; offset = NORMAL_VAR_BASE; break;
    case OPKIND_GLOBAL_STR: g_vec = &wc->g_strs; offset = STRING_VAR_BASE; break;
    case OPKIND_GLOBAL_CEV: g_vec = &wc->g_cevs; offset = CEV_BASE; break;
    case OPKIND_GLOBAL_UDB: g_vec = &wc->g_udbs; offset = UDB_BASE; break;
    case OPKIND_GLOBAL_CDB: g_vec = &wc->g_cdbs; offset = CDB_BASE; break;
    
    case OPKIND_IMM_STR:
    case OPKIND_INTERP:
        UNREACHABLE;
    }

    for (size_t i = 0; i < g_vec->count; i++) {
        if (sv_equals(wop.as.global.path, g_vec->at[i].path)
            && sv_equals(wop.as.global.name, g_vec->at[i].name)) {
                
            return offset + i;
        }
    }

    UNREACHABLE;
    return 0;
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
            assert(frag.as.offset <= CSELF_INT_MAX - CSELF_BASE);
            snprintf(buf, sizeof(buf), "\\cself[%zu]", frag.as.offset + CSELF_INT_BASE - CSELF_BASE);
            next = to_sv(buf);
            break;
        }
        case OPKIND_LOCAL_STR: {
            assert(frag.as.offset <= CSELF_STR_MAX - CSELF_BASE);
            snprintf(buf, sizeof(buf), "\\cself[%zu]", frag.as.offset + CSELF_STR_BASE - CSELF_BASE);
            next = to_sv(buf);
            break;
        }
        case OPKIND_TEMP_INT: {
            assert(wc->int_map.at[frag.as.offset] < CSELF_INT_MAX);
            snprintf(buf, sizeof(buf), "\\cself[%zu]", wc->int_map.at[frag.as.offset] - CSELF_BASE);
            next = to_sv(buf);
            break;
        }
        case OPKIND_TEMP_STR: {
            assert(wc->str_map.at[frag.as.offset] < CSELF_STR_MAX);
            snprintf(buf, sizeof(buf), "\\cself[%zu]", wc->str_map.at[frag.as.offset] - CSELF_BASE);
            next = to_sv(buf);
            break;
        }
        case OPKIND_GLOBAL_INT: {
            for (size_t j = 0; j < wc->g_ints.count; j++) {
                if (sv_equals(frag.as.global.path, wc->g_ints.at[j].path)
                    && sv_equals(frag.as.global.name, wc->g_ints.at[j].name)) {
                        
                    snprintf(buf, sizeof(buf), "\\v[%zu]", j);
                    next = to_sv(buf);
                }
            }
            UNREACHABLE;
            break;
        }
        case OPKIND_GLOBAL_STR: {
            for (size_t j = 0; j < wc->g_strs.count; j++) {
                if (sv_equals(frag.as.global.path, wc->g_strs.at[j].path)
                    && sv_equals(frag.as.global.name, wc->g_strs.at[j].name)) {
                        
                    snprintf(buf, sizeof(buf), "\\s[%zu]", j);
                    next = to_sv(buf);
                }
            }
            UNREACHABLE;
            break;
        }
        case OPKIND_GLOBAL_CEV:
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            next = frag.as.global.name;
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

static void update_map(VEC_int32_t *i_map, size_t i_top, VEC_int32_t *s_map, size_t s_top, WIROperand wop) {
    if (wop.kind == OPKIND_TEMP_INT) {
        // If temporary has already been given a concrete address, skip.
        if (i_map->at[wop.as.offset] >= RC_THRESHOLD) return;

        i_map->at[wop.as.offset] += CSELF_INT_BASE + i_top;

        // Crash when overflowing the CSelf space for now.
        assert(i_map->at[wop.as.offset] <= CSELF_INT_MAX);
    } else if (wop.kind == OPKIND_TEMP_STR) {
        if (s_map->at[wop.as.offset] >= RC_THRESHOLD) return;

        s_map->at[wop.as.offset] += CSELF_STR_BASE + s_top;

        assert(s_map->at[wop.as.offset] <= CSELF_STR_MAX);
    }
}

// Allocate offsets to temporaries.
// Essentially, this is linear register allocation but with an
// infinite number of physical registers.
// (https://en.wikipedia.org/wiki/Register_allocation#Linear_scan)
static VEC_int32_t reg_alloc(Arena *arena, VEC_Interval *its) {
    VEC_int32_t map = VEC_EMPTY;
    for (size_t i = 0; i < its->count; i++)
        VEC_PUSH(map, 0, arena);
    
    // Whether or not a register is active.
    VEC_DEF(bool);
    VEC_bool regs = VEC_EMPTY;
    for (size_t i = 0; i < its->count; i++)
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

// Assigns concrete addresses to temporaries.
static void temp_alloc_pass(Arena *arena, WIRCev *wcev, VEC_int32_t *i_map, VEC_int32_t *s_map) {
    // Compute liveness intervals of all temporaries.
    size_t n_i_temps = wcev->n_temp_ints;
    size_t n_s_temps = wcev->n_temp_strs;

    VEC_Interval i_its = VEC_EMPTY;
    for (size_t i = 0; i < n_i_temps; i++)
        VEC_PUSH(i_its, ((Interval){ .id = i,
            .start = (size_t)-1, .end = (size_t)-1 }), arena);
    
    VEC_Interval s_its = VEC_EMPTY;
    for (size_t i = 0; i < n_s_temps; i++)
        VEC_PUSH(s_its, ((Interval){ .id = i,
            .start = (size_t)-1, .end = (size_t)-1 }), arena);

    for (size_t i = 0; i < wcev->insts.count; i++) {
        WIRInst *wirinst = wcev->insts.at[i];
        switch (wirinst->kind) {
        case _WIRInst_TOMBSTONE:
        case _WIRInst_PushInt:
        case _WIRInst_PushStr:
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
        case _WIRInst_IfBegin: {
            WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wirinst;
            update_interval(&i_its, &s_its, i, inst->cond);
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
            update_interval(&i_its, &s_its, i, inst->db_type);
            update_interval(&i_its, &s_its, i, inst->db_data);
            update_interval(&i_its, &s_its, i, inst->db_field);
            break;
        }
        case _WIRInst_DBStore: {
            WIRInst_DBStore *inst = (WIRInst_DBStore *)wirinst;
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

    *i_map = reg_alloc(arena, &i_its);
    *s_map = reg_alloc(arena, &s_its);

    // Now do one more pass of the code, keeping track of the
    // state of the local variable stack, to assign actual
    // addresses to each temporary. Addresses are assigned such
    // that they are always right above the local stack.
    size_t i_top = 0;
    size_t s_top = 0;
    for (size_t i = 0; i < wcev->insts.count; i++) {
        WIRInst *wirinst = wcev->insts.at[i];
        switch (wirinst->kind) {
        case _WIRInst_PushInt:
            i_top++;
            break;
        case _WIRInst_PushStr:
            s_top++;
            break;
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
            update_map(i_map, i_top, s_map, s_top, inst->dest);
            update_map(i_map, i_top, s_map, s_top, inst->a);
            update_map(i_map, i_top, s_map, s_top, inst->b);
            break;
        }
        case _WIRInst_IfBegin: {
            WIRInst_IfBegin *inst = (WIRInst_IfBegin *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->cond);
            break;
        }
        case _WIRInst_LoopBeginN: {
            WIRInst_LoopBeginN *inst = (WIRInst_LoopBeginN *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->count);
            break;
        }
        case _WIRInst_Call: {
            WIRInst_Call *inst = (WIRInst_Call *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->dest);
            for (size_t arg = 0; arg < inst->args.count; arg++)
                update_map(i_map, i_top, s_map, s_top, inst->args.at[arg]);
            break;
        }
        case _WIRInst_ReturnVal: {
            WIRInst_ReturnVal *inst = (WIRInst_ReturnVal *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->val);
            break;
        }
        case _WIRInst_DBLoad: {
            WIRInst_DBLoad *inst = (WIRInst_DBLoad *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->db_type);
            update_map(i_map, i_top, s_map, s_top, inst->db_data);
            update_map(i_map, i_top, s_map, s_top, inst->db_field);
            break;
        }
        case _WIRInst_DBStore: {
            WIRInst_DBStore *inst = (WIRInst_DBStore *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->db_type);
            update_map(i_map, i_top, s_map, s_top, inst->db_data);
            update_map(i_map, i_top, s_map, s_top, inst->db_field);
            break;
        }
        case _WIRInst_StrAssign: {
            WIRInst_StrAssign *inst = (WIRInst_StrAssign *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->dest);
            update_map(i_map, i_top, s_map, s_top, inst->src);
            break;
        }
        case _WIRInst_Label: {
            WIRInst_Label *inst = (WIRInst_Label *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->name);
            break;
        }
        case _WIRInst_Goto: {
            WIRInst_Goto *inst = (WIRInst_Goto *)wirinst;
            update_map(i_map, i_top, s_map, s_top, inst->name);
            break;
        }
        case _WIRInst_TOMBSTONE:
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

    return ;
}

static void push_binop_command(WIRCompiler *wc, int32_t dest, int32_t a, int32_t b, int cmd_var_op) {
    VEC_int32_t i_vec = VEC_EMPTY;
    
    VEC_PUSH(i_vec, dest, wc->arena);
    VEC_PUSH(i_vec, a, wc->arena);
    VEC_PUSH(i_vec, b, wc->arena);
    VEC_PUSH(i_vec, cmd_var_op, wc->arena);

    cev_push_cmd(wc->cev, CMD_VAR, wc->indent, i_vec, (VEC_StringView)VEC_EMPTY);
}

static void compile_binop(WIRCompiler *wc, WIRInst_Binop *inst, int cmd_var_op) {
    push_binop_command(wc,
        resolve(wc, inst->dest), resolve(wc, inst->a), resolve(wc, inst->b),
        cmd_var_op);
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

static void compile_inst(WIRCompiler *wc, WIRInst *wi) {
    switch (wi->kind) {
    case _WIRInst_Binop: {
        WIRInst_Binop *inst = (WIRInst_Binop *)wi;
        switch (inst->op) {
        case WIR_ADD: compile_binop(wc, inst, VAR_OP_PLUS); break;
        case WIR_SUB: compile_binop(wc, inst, VAR_OP_MINUS); break;
        case WIR_MUL: compile_binop(wc, inst, VAR_OP_TIMES); break;
        case WIR_DIV: compile_binop(wc, inst, VAR_OP_DIV); break;
        case WIR_MOD: compile_binop(wc, inst, VAR_OP_MOD); break;
        case WIR_XOR: compile_binop(wc, inst, VAR_OP_XOR); break;
        case WIR_LSH: compile_binop(wc, inst, VAR_OP_LSHIFT); break;
        case WIR_AND: compile_binop(wc, inst, VAR_OP_AND); break;
        case WIR_OR:  compile_binop(wc, inst, VAR_OP_OR); break;
        
        case WIR_EQ: UNIMPLEMENTED;
        case WIR_NEQ: UNIMPLEMENTED;
        case WIR_LT: UNIMPLEMENTED;
        case WIR_LTE: UNIMPLEMENTED;
        case WIR_GT: UNIMPLEMENTED;
        case WIR_GTE: UNIMPLEMENTED;
        case WIR_LAND: UNIMPLEMENTED;
        case WIR_LOR: UNIMPLEMENTED;
        }
        break;                
    }
    case _WIRInst_StrAssign: {
        WIRInst_StrAssign *inst = (WIRInst_StrAssign *)wi;
        push_str_command(wc, resolve(wc, inst->dest), inst->src);
        break;
    }
    // TODO
    case _WIRInst_IfBegin: {
        cev_push_simple_cmd(wc->cev, CMD_IF_INT, wc->indent);
        
        wc->indent++;
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
        //       Temporaries use a map instead of the offset field, so they
        //       have to be handled separately. And so on.

        int32_t *target = &wc->cev->RETURN_VAL_TARGET;

        switch (inst->val.kind) {
            case OPKIND_IMM_INT:
            case OPKIND_LOCAL_INT:
            case OPKIND_TEMP_INT:
            case OPKIND_GLOBAL_INT:
            case OPKIND_GLOBAL_CEV:
                *target = 99;
                push_binop_command(wc, *target + CSELF_BASE,
                    resolve(wc, inst->val), 0, VAR_OP_PLUS);
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
            case OPKIND_GLOBAL_UDB:
            case OPKIND_GLOBAL_CDB:
                UNREACHABLE;
        }

        cev_push_simple_cmd(wc->cev, CMD_RETURN, wc->indent);
        break;
    }
    case _WIRInst_ReturnVoid: {
        cev_push_simple_cmd(wc->cev, CMD_RETURN, wc->indent);
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
    case _WIRInst_TOMBSTONE: break;
    }        
}

// For the current WIRCev, if the temporary result of a calculation
// is moved directly into a variable, rewrite to store the result
// directly into the variable.
static void temp_copy_propagation_pass(WIRCev *wcev) {
    for (size_t i = 0; i < wcev->insts.count; i++) {
        if (wcev->insts.at[i]->kind != _WIRInst_Binop)
            continue;
        
        WIRInst_Binop *inst = (WIRInst_Binop *)wcev->insts.at[i]; 

        if (inst->op != WIR_ADD) continue;
        if (inst->a.kind != OPKIND_TEMP_INT) continue;
        if (inst->b.kind != OPKIND_IMM_INT) continue;
        if (inst->b.as.imm_int != 0) continue;

        assert(i > 0);
        WIRInst *prev = wcev->insts.at[i - 1];

        if (prev->kind == _WIRInst_Binop) {
            WIRInst_Binop *p = (WIRInst_Binop *)prev;
            if (p->dest.kind == OPKIND_TEMP_INT
                && p->dest.as.offset == inst->a.as.offset) {

                p->dest = inst->dest;
                inst->base.kind = _WIRInst_TOMBSTONE;
            }
        }
    }
}


static void compile_wir(WIRCompiler *wc, Module *mod) {
    WIR *wir = mod->wir;

    // Dump all the top-level symbols in the current file
    // into the global list.
    {
        #define LOAD_SYMBOLS(dst, src) do { \
            for (size_t i = 0; i < src.count; i++) { \
                VEC_PUSH(dst, ((GlobalEntry){ \
                    .path = mod->source->path, \
                    .name = src.at[i].name \
                }), wc->arena); \
            }} while (0)
        
        LOAD_SYMBOLS(wc->g_ints, wir->g_ints);
        LOAD_SYMBOLS(wc->g_strs, wir->g_strs);
        LOAD_SYMBOLS(wc->g_cevs, wir->g_cevs);
        LOAD_SYMBOLS(wc->g_udbs, wir->g_udbs);
        LOAD_SYMBOLS(wc->g_cdbs, wir->g_cdbs);
        
        #undef LOAD_SYMBOLS
    }
    print_wir(mod->wir);
    
    // Process every `WIRCev`.
    for (size_t i = 0; i < wir->g_cevs.count; i++) {
        WIRCev *wcev = &wir->g_cevs.at[i];

        // First, apply transformations to the code.
        temp_copy_propagation_pass(wcev);
        disable_rc_pass(wc->arena, wcev);
        
        // Map concrete addresses to temporaries.
        temp_alloc_pass(wc->arena, wcev, &wc->int_map, &wc->str_map);

        // Then compile the code into commands.
        CommonEvent cev;
        cev_init(&cev, wc->arena);
        cev.COMMON_NAME = wcev->name;
        wc->cev = &cev;
        wc->indent = 0;

        for (size_t j = 0; j < wcev->insts.count; j++) {
            compile_inst(wc, wcev->insts.at[j]);
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

GameData wir_pass(VEC_Module *modules, Arena *arena) {
    WIRCompiler wc = {
        .g_ints = VEC_EMPTY,
        .g_strs = VEC_EMPTY,
        .g_cevs = VEC_EMPTY,
        .g_udbs = VEC_EMPTY,
        .g_cdbs = VEC_EMPTY,
        .arena = arena
    };
    gd_init(&wc.gd);
    
    for (size_t i = 0; i < modules->count; i++) {
        compile_wir(&wc, &modules->at[i]);
    }

    // Assign entry point.
    for (size_t i = 0; i < wc.g_cevs.count; i++) {
        if (sv_equals(wc.g_cevs.at[i].name, SV("main"))) {
            wc.gd.entry = 500000 + i;
        }
    }

    DB db;
    db_init(&db);
    VEC_PUSH(wc.gd.cdb, db, arena);

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
        case OPKIND_GLOBAL_UDB:
            printf(" $GUDB[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
        case OPKIND_GLOBAL_CDB:
            printf(" $GCDB[" SV_FMT ":" SV_FMT "]",
                SV_FMT_VAL(wop.as.global.path), SV_FMT_VAL(wop.as.global.name));
            return;
    }
}

void print_wir(WIR *wir) {
    for (size_t cev = 0; cev < wir->g_cevs.count; cev++) {
        VEC_PTR_WIRInst arr = wir->g_cevs.at[cev].insts;

        size_t stack_i = 0;
        size_t stack_s = 0;

        printf(SV_FMT ":\n", SV_FMT_VAL(wir->g_cevs.at[cev].name));

        for (size_t i = 0; i < arr.count; i++) {
            WIRInst *inst = arr.at[i];
            switch (inst->kind) {
            case _WIRInst_PushInt:
                printf("pushi \t\t\t; (max i: %zu)", stack_i);
                stack_i++;
                break;
            case _WIRInst_PushStr:
                printf("pushs \t\t\t; (max s: %zu)", stack_s);
                stack_i++;
                break;
            case _WIRInst_PopIntN: {
                WIRInst_PopIntN *in = (WIRInst_PopIntN *)inst;
                stack_i -= in->n;
                printf("popi %zu \t\t\t", in->n);
                if (stack_i == 0)
                    printf("; (max i: -)");
                else
                    printf("; (max i: %zu)", stack_i);
                break;
            }
            case _WIRInst_PopStrN: {
                WIRInst_PopStrN *in = (WIRInst_PopStrN *)inst;
                stack_s -= in->n;
                printf("pops %zu \t\t\t", in->n);
                if (stack_s == 0)
                    printf("; (max s: -)");
                else
                    printf("; (max s: %zu)", stack_s);
                break;
            }
            case _WIRInst_Binop: {
                printf("binop");
                WIRInst_Binop *in = (WIRInst_Binop *)inst;
                print_wop(in->dest);
                printf(" %d", in->op);
                print_wop(in->a);
                print_wop(in->b);
                break;
            }
            case _WIRInst_ReturnVal: {
                printf("ret");
                WIRInst_ReturnVal *in = (WIRInst_ReturnVal *)inst;
                print_wop(in->val);
                break;
            }
            case _WIRInst_Call: {
                printf("call");
                WIRInst_Call *in = (WIRInst_Call *)inst;
                print_wop(in->dest);
                print_wop(in->cev);
                for (size_t arg = 0; arg < in->args.count; arg++) {
                    print_wop(in->args.at[arg]);
                }
                break;
            }
            case _WIRInst_Cmd: {
                printf("cmd");
                WIRInst_Cmd *in = (WIRInst_Cmd *)inst;
                printf(" %d %d", in->op, in->open_close);
                for (size_t arg = 0; arg < in->iargs.count; arg++)
                    print_wop(in->iargs.at[arg]);
                for (size_t arg = 0; arg < in->sargs.count; arg++)
                    print_wop(in->sargs.at[arg]);
                break;
            }
            case _WIRInst_TOMBSTONE:
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

    for (size_t i = 0; i < wir->g_cdbs.count; i++) {
        WIRDB *db = &wir->g_cdbs.at[i];

        printf("(DB) " SV_FMT ":\n", SV_FMT_VAL(db->name));
        for (size_t j = 0; j < db->fields.count; j++) {
            WIRVar *field = &db->fields.at[j];
            
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
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            return false;
        case OPKIND_LOCAL_INT:
        case OPKIND_LOCAL_STR:
            return true;
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
        case OPKIND_GLOBAL_UDB:
        case OPKIND_GLOBAL_CDB:
            return true;
    }
    UNREACHABLE;
    return false;
}