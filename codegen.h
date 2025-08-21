#pragma once

#include <iostream>
#include <fstream>
#include <stack>
#include "visitor.h"
#include "ast.h"
#include "commonevent.h"
#include "command.h"
#include "db.h"
#include "gamedata.h"

class Codegen : public Visitor {
public:
    Codegen(int use_globals) : use_globals(use_globals) {
        if (use_globals) {
            BASE_INT_SP = VAR_THRESHOLD - 1;
            MAX_INT_SP = VAR_THRESHOLD + 99999;
            BASE_STR_SP = STRVAR_THRESHOLD - 1;
            MAX_STR_SP = STRVAR_THRESHOLD + 99999;
        } else {
            BASE_INT_SP = CSELF_THRESHOLD + 9;
            MAX_INT_SP = MAX_CSELF_REF;
            BASE_STR_SP = CSELF_THRESHOLD + 5;
            MAX_STR_SP = CSELF_THRESHOLD + 9;
        }
    }

    GameData gen(std::vector<Stmt*> &program) {
        for (Stmt* s : program) {
            s->accept(this);
        }

        // editor will crash if there are 0 dbs
        if (gd.cdbs.size() == 0) {
            gd.cdbs.push_back(DB());
        }

        return gd;
    }

    bool failed() { return had_error; }

    // statements
    void visit_FunctionStmt(FunctionStmt* stmt) override {
        if (stmt->is_inline) return;

        if (stmt->name == "main") gd.entry = stmt->sym->ref;

        int_sp = BASE_INT_SP;
        str_sp = BASE_STR_SP;
        for (WodType t : stmt->sym->arg_types) {
            if (t == TYPE_INT) push_int();
            else if (t == TYPE_STR) push_str();
            else assert(false);
        }

        begin_frame();

        gd.cevs.push_back(CommonEvent());
        current_cev = &gd.cevs.back();
        current_cev->COMMON_ID = cev_index++;

        for (Stmt* s : stmt->body) {
            s->accept(this);
        }

        // for text file common events, if the last line is not an empty line then loading will fail
        current_cev->add_cmd(CMD_EMPTY);

        end_frame();
    }

    void visit_BlockStmt(BlockStmt* stmt) override {
        begin_frame();
        for (Stmt* s : stmt->stmts) {
            s->accept(this);
        }
        end_frame();
    }

    void visit_ReturnStmt(ReturnStmt* stmt) override {
        if (!inline_funcs.empty()) inline_return(stmt);
        else normal_return(stmt);
    }

    void normal_return(ReturnStmt* stmt) {
        if (!stmt->expr) {
            current_cev->add_cmd(CMD_RETURN);
            return;
        }
        
        begin_frame();
        WolfValue v = try_cdb_eval(stmt->expr);
        if (current_cev->RETURN_VAL_TARGET == -1)
            current_cev->RETURN_VAL_TARGET = 99;
        switch (stmt->expr->type.ty) {
            case TYPE_INT:
                cmd_arith(WolfValue(WT_NUMREF, CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET), v);
                break;
            case TYPE_STR:
                cmd_string(WolfValue(WT_STRREF, CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET), v);
                break;
        }
        current_cev->add_cmd(CMD_RETURN);
        end_frame();
    }

    void inline_return(ReturnStmt* stmt) {
        if (!stmt->expr) {
            current_cev->add_cmd(CMD_JUMP, {}, {std::to_string(inline_funcs.top().exit_label)});
            return;
        }
        begin_frame();
        WolfValue v = try_cdb_eval(stmt->expr);
        switch (stmt->expr->type.ty) {
            case TYPE_INT:
                cmd_arith(inline_funcs.top().ret, v);
                break;
            case TYPE_STR:
                cmd_string(inline_funcs.top().ret, v);
                break;
        }
        current_cev->add_cmd(CMD_JUMP, {}, {std::to_string(inline_funcs.top().exit_label)});
        end_frame();
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        begin_frame();
        eval(stmt->expr);
        end_frame();
    }

    void visit_VarStmt(VarStmt* stmt) override {
        if (stmt->is_const) return;

        if (stmt->sym->type == TYPE_INTARR) {
            WolfValue base = push_int();
            stmt->sym->ref = base.v;
            for (size_t i = 1; i < stmt->sym->type.i; i++)
                push_int();
            return;
        }

        WolfValue lhs;
        if (stmt->sym->type == TYPE_INT)
            lhs = push_int();
        else
            lhs = push_str();
        stmt->sym->ref = lhs.v;

        if (!stmt->initializer) return;

        begin_frame();
        WolfValue initial = eval(stmt->initializer);
        if (initial.wt == WT_DB) {
            cmd_cdb_get(lhs, initial, DB_ASSIGN_EQ);
        } else if (stmt->sym->type == TYPE_INT) {
            cmd_arith(lhs, initial);
        } else {
            cmd_string(lhs, initial);
        }
        end_frame();
    }

    void visit_IfStmt(IfStmt* stmt) override {
        begin_frame();
        WolfValue cond = try_cdb_eval(stmt->condition);
        if (stmt->else_branch)
            cmd_int_if(true, cond, WolfValue(WT_NUMLIT, 0), IF_INT_OP_NEQ);
        else
            cmd_int_if(false, cond, WolfValue(WT_NUMLIT, 0), IF_INT_OP_NEQ);
        end_frame();

        begin_frame();
        stmt->then_branch->accept(this);
        end_frame();
        if (stmt->else_branch) {
            cmd_else();
            begin_frame();
            stmt->else_branch->accept(this);
            end_frame();
        }
        cmd_end_if();
    }

    void visit_LoopStmt(LoopStmt* stmt) override {
        begin_frame();
        if (stmt->count) {
            WolfValue count = try_suppress(try_cdb_eval(stmt->count));
            current_cev->add_cmd(CMD_LOOP_COUNT, {count.v}, {});
        } else {
            current_cev->add_cmd(CMD_LOOP, {}, {});
        }
        end_frame();

        begin_frame();
        current_cev->indent();
        stmt->body->accept(this);
        current_cev->outdent();
        end_frame();

        current_cev->add_cmd(CMD_LOOP_END, {}, {});
    }

    void visit_ContinueStmt(ContinueStmt* stmt) override {
        current_cev->add_cmd(CMD_CONTINUE);
    }

    void visit_BreakStmt(BreakStmt* stmt) override {
        current_cev->add_cmd(CMD_BREAK);
    }

    void visit_CmdStmt(CmdStmt* stmt) override {
        std::vector<int32_t> int_fields;
        std::vector<std::string> str_fields;

        int32_t cmd_id = stmt->cmd_id->const_int;
        begin_frame();
        for (Expr* e: stmt->int_fields)
            int_fields.push_back(try_cdb_eval(e).v);
        for (Expr* e: stmt->str_fields) {
            WolfValue v = try_cdb_eval(e);
            if (v.wt == WT_STRREF) {
                if (use_globals) {
                    str_fields.push_back("\\v[" + std::to_string(v.v - VAR_THRESHOLD) + "]");
                } else {
                    str_fields.push_back("\\cself[" + std::to_string(v.v - CSELF_THRESHOLD) + "]");
                }
            }
            else
                str_fields.push_back(v.string_lit);
        }

        current_cev->add_cmd(cmd_id, int_fields, str_fields);
        end_frame();
    }

    void visit_CdbStmt(CdbStmt* stmt) override {
        gd.cdbs.push_back(DB());
        for (VarStmt* vs : stmt->fields) {
            if (vs->type == TYPE_INTARR) {
                for (size_t i = 0; i < vs->type.i; i++) {
                    gd.cdbs.back().properties.push_back(DB::Property());
                    gd.cdbs.back().properties.back().type = DB::PROP_INT;
                }
            } else {
                DB::Property p;
                if (vs->type == TYPE_INT)
                    p.type = DB::PROP_INT;
                else
                    p.type = DB::PROP_STR;
                gd.cdbs.back().properties.push_back(p);
            }
        }
        gd.cdbs.back().TYPE_ID = cdb_index++;
    }

    // expressions
    void visit_AssignExpr(AssignExpr* expr) override {
        WolfValue lhs = eval(expr->lhs);
        WolfValue rhs = eval(expr->rhs);
        
        DBFlag db_assign;
        if (lhs.wt == WT_DB || rhs.wt == WT_DB) {
            switch (expr->op) {
                case AssignExpr::EQUAL:         db_assign = DB_ASSIGN_EQ; break;
                case AssignExpr::PLUS_EQUAL:    db_assign = DB_ASSIGN_PLUS_EQ; break;
                case AssignExpr::MINUS_EQUAL:   db_assign = DB_ASSIGN_MINUS_EQ; break;
                case AssignExpr::TIMES_EQUAL:   db_assign = DB_ASSIGN_TIMES_EQ; break;
                case AssignExpr::DIV_EQUAL:     db_assign = DB_ASSIGN_DIV_EQ; break;
                case AssignExpr::MOD_EQUAL:     db_assign = DB_ASSIGN_MOD_EQ; break;
            }
        }

        begin_frame();
        if (rhs.wt == WT_DB) {
            if (lhs.wt == WT_DB) {
                // using a db reference directly is slow for some reason, so in the case
                // that we assign a db to another db, grab the rhs into a temporary first
                WolfValue new_rhs;
                if (expr->rhs->type == TYPE_INT) {
                    new_rhs = push_int();
                } else {
                    new_rhs = push_str();
                }
                cmd_cdb_get(new_rhs, rhs, DB_ASSIGN_EQ);
                rhs = new_rhs;
                cmd_cdb_put(lhs, rhs, db_assign);
            } else {
                cmd_cdb_get(lhs, rhs, db_assign);
            }
        } else if (lhs.wt == WT_DB) {
            cmd_cdb_put(lhs, rhs, db_assign);
        } else if (lhs.wt == WT_NUMREF) {
            ArithFlag arith_assign;
            switch (expr->op) {
                case AssignExpr::EQUAL:         arith_assign = ARITH_ASSIGN_EQ; break;
                case AssignExpr::PLUS_EQUAL:    arith_assign = ARITH_ASSIGN_PLUS_EQ; break;
                case AssignExpr::MINUS_EQUAL:   arith_assign = ARITH_ASSIGN_MINUS_EQ; break;
                case AssignExpr::TIMES_EQUAL:   arith_assign = ARITH_ASSIGN_TIMES_EQ; break;
                case AssignExpr::DIV_EQUAL:     arith_assign = ARITH_ASSIGN_DIV_EQ; break;
                case AssignExpr::MOD_EQUAL:     arith_assign = ARITH_ASSIGN_MOD_EQ; break;
            }
            cmd_arith(lhs, rhs, WolfValue(WT_NUMLIT, 0), arith_assign);
        } else {
            StringFlag assign;
            if (expr->op == AssignExpr::PLUS_EQUAL)
                assign = STRING_ASSIGN_PLUS_EQ;
            else
                assign = STRING_ASSIGN_EQ;

            cmd_string(lhs, rhs, assign);
        }
        end_frame();
    }

    void visit_VariableExpr(VariableExpr* expr) override {
        if (try_const(expr)) return;
        if (!inline_funcs.empty()) inline_var(expr);
        else normal_var(expr);
    }

    void normal_var(VariableExpr* expr) {
        Symbol* sym = expr->sym;
        if (sym->type == TYPE_INT || sym->type == TYPE_INTARR)
            expr_return = WolfValue(WT_NUMREF, sym->ref);
        else
            expr_return = WolfValue(WT_STRREF, sym->ref);
    }

    void inline_var(VariableExpr* expr) {
        for (size_t i = 0; i < inline_funcs.top().fn->params.size(); i++) {
            if (inline_funcs.top().fn->params.at(i)->sym == expr->sym) {
                expr_return = inline_funcs.top().args.at(i);
                return;
            }
        }
        normal_var(expr);
    }

    void visit_ArrayExpr(ArrayExpr* expr) override {
        if (!inline_funcs.empty()) {
            for (size_t i = 0; i < inline_funcs.top().fn->params.size(); i++) {
                if (inline_funcs.top().fn->params.at(i)->sym == expr->sym) {
                    expr_return = inline_funcs.top().args.at(i);
                    if (expr_return.wt == WT_DB) {
                        expr_return.db_prop += expr->index->const_int;
                    } else {
                        expr_return.v += expr->index->const_int;
                    }
                    return;
                }
            }
        }
        expr_return = WolfValue(WT_NUMREF, expr->sym->ref + expr->index->const_int);
    }

    void visit_CdbExpr(CdbExpr* expr) override {
        Symbol* sym = expr->sym->cdb_fields->get(expr->property);
        int32_t prop_index = sym->ref;
        if (sym->type == TYPE_INTARR && expr->arr_index) {
            prop_index += expr->arr_index->const_int;
        }
        int32_t index;
        if (expr->data_index->is_const) {
            index = expr->data_index->const_int;
        } else {
            index = try_cdb_eval(expr->data_index).v;
        }
        expr_return = WolfValue(
            expr->sym->ref,
            index,
            prop_index
        );
    }

    void visit_BinaryExpr(BinaryExpr* expr) override {
        if (try_const(expr)) return;

        switch (expr->op) {
            case BinaryExpr::LOGIC_OR:
            case BinaryExpr::LOGIC_AND:
                binary_logical_expr(expr);
                break;
            case BinaryExpr::GT:
            case BinaryExpr::GTE:
            case BinaryExpr::LT:
            case BinaryExpr::LTE:
            case BinaryExpr::EQ:
            case BinaryExpr::NEQ:
                binary_comp_expr(expr);
                break;
            default:
                binary_normal_expr(expr);
                break;
        }
    }

    void binary_logical_expr(BinaryExpr* expr) {
        if (try_const(expr)) return;
        WolfValue temp = push_int();

        begin_frame();
        WolfValue left = try_cdb_eval(expr->left);
        end_frame();

        begin_frame();
        WolfValue right;
        switch (expr->op) {
            case BinaryExpr::LOGIC_AND:
                cmd_int_if(true, left, WolfValue(WT_NUMLIT, 0), IF_INT_OP_NEQ);
                    right = try_cdb_eval(expr->right);
                    cmd_int_if(true, right, WolfValue(WT_NUMLIT, 0), IF_INT_OP_NEQ);
                    cmd_arith(temp, 1);
                    cmd_else();
                    cmd_arith(temp, 0);
                    cmd_end_if();
                cmd_else();
                    cmd_arith(temp, 0);
                cmd_end_if();
                break;
            case BinaryExpr::LOGIC_OR:
                cmd_int_if(true, left, WolfValue(WT_NUMLIT, 0), IF_INT_OP_NEQ);
                    cmd_arith(temp, 1);
                cmd_else();
                    right = try_cdb_eval(expr->right);
                    cmd_int_if(true, right, WolfValue(WT_NUMLIT, 0), IF_INT_OP_NEQ);
                    cmd_arith(temp, 1);
                    cmd_else();
                    cmd_arith(temp, 0);
                    cmd_end_if();
                cmd_end_if();
                break;
        }
        end_frame();

        expr_return = temp;
    }

    void binary_comp_expr(BinaryExpr* expr) {
        if (try_const(expr)) return;
        WolfValue temp = push_int();

        begin_frame();
        WolfValue left = try_cdb_eval(expr->left);
        WolfValue right = try_cdb_eval(expr->right);
        IfIntBranchFlag op;
        switch (expr->op) {
            case BinaryExpr::EQ:    op = IF_INT_OP_EQ; break;
            case BinaryExpr::NEQ:   op = IF_INT_OP_NEQ; break;
            case BinaryExpr::GT:    op = IF_INT_OP_GT; break;
            case BinaryExpr::GTE:   op = IF_INT_OP_GTE; break;
            case BinaryExpr::LT:    op = IF_INT_OP_LT; break;
            case BinaryExpr::LTE:   op = IF_INT_OP_LTE; break;
        }
        
        cmd_int_if(true, left, right, op);
        cmd_arith(temp, 1);
        cmd_else();
        cmd_arith(temp, 0);
        cmd_end_if();
        end_frame();

        expr_return = temp;
    }

    void binary_normal_expr(BinaryExpr* expr) {
        if (try_const(expr)) return;
        WolfValue temp = push_int();

        begin_frame();
        WolfValue left = try_cdb_eval(expr->left);
        WolfValue right = try_cdb_eval(expr->right);
        ArithFlag op;
        switch (expr->op) {
            case BinaryExpr::ADD:       op = ARITH_OP_PLUS; break;
            case BinaryExpr::SUB:       op = ARITH_OP_MINUS; break;
            case BinaryExpr::MUL:       op = ARITH_OP_TIMES; break;
            case BinaryExpr::DIV:       op = ARITH_OP_DIV; break;
            case BinaryExpr::MODULO:    op = ARITH_OP_MOD; break;
            case BinaryExpr::BIT_OR:    op = ARITH_OP_OR; break;
            case BinaryExpr::BIT_AND:   op = ARITH_OP_AND; break;
            case BinaryExpr::BIT_XOR:   op = ARITH_OP_XOR; break;
            case BinaryExpr::LSHIFT:    op = ARITH_OP_LSHIFT; break;
            case BinaryExpr::RSHIFT:
                op = ARITH_OP_LSHIFT;
                WolfValue new_right = push_int();
                cmd_arith(new_right, WolfValue(WT_NUMLIT, 0), right, ARITH_OP_MINUS);
                right = new_right;
                break;
        }
        cmd_arith(temp, left, right, op);
        end_frame();

        expr_return = temp;
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        if (try_const(expr)) return;
        WolfValue temp = push_int();

        begin_frame();
        WolfValue right = try_cdb_eval(expr->right);
        switch (expr->op) {
            case UnaryExpr::LOGIC_NOT:
                cmd_int_if(true, right, WolfValue(WT_NUMLIT, 0), IF_INT_OP_EQ);
                cmd_arith(temp, 1);
                cmd_else();
                cmd_arith(temp, 0);
                cmd_end_if();
                break;
            case UnaryExpr::MINUS:
                cmd_arith(temp, WolfValue(WT_NUMLIT, 0), right, ARITH_OP_MINUS);
                break;
            case UnaryExpr::ADDRESS_OF:
                temp = right;
                break;
        }
        end_frame();

        expr_return = temp;
    }

    void visit_CallExpr(CallExpr* expr) override {
        if (expr->sym->inline_function) inline_call(expr);
        else normal_call(expr);
    }
    
    void inline_call(CallExpr* expr) {
        WolfValue retval = push_int();

        begin_frame();

        std::vector<WolfValue> args;
        for (Expr* arg : expr->args) {
            WolfValue v;
            if (arg->type == TYPE_INTARR)
                v = eval(arg);
            else {
                v = try_cdb_eval(arg);
                // for variables passed by value, do a copy here to
                // simulate copying of variables during a normal function call
                if (v.wt == WT_NUMREF /*|| v.wt == WT_NUMLIT*/) {
                    WolfValue v2 = push_int();
                    cmd_arith(v2, v);
                    v = v2;
                } else if (v.wt == WT_STRREF /*|| v.wt == WT_STRLIT*/) {
                    WolfValue v2 = push_str();
                    cmd_string(v2, v);
                    v = v2;
                }
            }
            args.push_back(v);
        }

        assert(inline_funcs.size() < 500); // currently no recursion detection
        inline_funcs.push(InlineFrame(expr->sym->inline_function, retval, args, new_label()));
        for (Stmt* s : expr->sym->inline_function->body) {
            s->accept(this);
        }
        current_cev->add_cmd(CMD_LABEL, {}, {std::to_string(inline_funcs.top().exit_label)});
        inline_funcs.pop();

        end_frame();

        expr_return = retval;
    }

    void normal_call(CallExpr* expr) {
        WolfValue temp = push_int();

        begin_frame();
        int32_t cev_ref = expr->sym->ref;
        std::vector<int32_t> int_args;
        std::vector<int32_t> strref_args;
        std::vector<std::string> str_args = {""};
        int32_t n_int_args = 0;
        int32_t n_str_args = 0;
        int32_t strlit_flags = 0;
        for (Expr* arg : expr->args) {
            WolfValue result = try_suppress(try_cdb_eval(arg));
            switch (result.wt) {
                case WT_NUMLIT:
                case WT_NUMREF:
                    int_args.push_back(result.v);
                    n_int_args++;
                    break;
                case WT_STRREF:
                    strref_args.push_back(result.v);
                    str_args.push_back("");
                    n_str_args++;
                    break;
                case WT_STRLIT:
                    strref_args.push_back(0);
                    str_args.push_back(result.string_lit);
                    strlit_flags |= (1 << n_str_args);
                    n_str_args++;
                    break;
            }
        }
        int32_t flags =
            n_int_args
            | n_str_args << 4
            | strlit_flags << 12
            | CALL_STORES_RETURN;
        std::vector<int32_t> cmd_int_fields{cev_ref, flags};
        cmd_int_fields.insert(cmd_int_fields.end(), int_args.begin(), int_args.end());
        cmd_int_fields.insert(cmd_int_fields.end(), strref_args.begin(), strref_args.end());
        cmd_int_fields.push_back(temp.v);
        if (strlit_flags)
            current_cev->add_cmd(CMD_CALL_ID, cmd_int_fields, str_args);
        else
            current_cev->add_cmd(CMD_CALL_ID, cmd_int_fields, {});
        end_frame();

        expr_return = temp;
    }

    void visit_IntLiteralExpr(IntLiteralExpr* expr) override {
        expr_return = WolfValue(WT_NUMLIT, expr->value);
    }
    
    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {
        expr_return = WolfValue(expr->value);
    }

    void visit_FStringExpr(FStringExpr* expr) override {
        std::string out;
        for (FStringExpr::Fragment& f : expr->frags) {
            if (f.expr) {
                WolfValue v = try_cdb_eval(f.expr);
                switch (v.wt) {
                    case WT_NUMLIT:
                        out += std::to_string(v.v);
                        break;
                    case WT_STRLIT:
                        out += v.string_lit;
                        break;
                    case WT_NUMREF:
                    case WT_STRREF:
                        if (use_globals) {
                            if (v.wt == WT_STRREF) error("can't currently print string variables when using globals");
                            out += "\\v[" + std::to_string(v.v - VAR_THRESHOLD) + "]";
                        } else {
                            out += "\\cself[" + std::to_string(v.v - CSELF_THRESHOLD) + "]";
                        }
                        break;
                }
            } else out += f.str;
        }
        expr_return = WolfValue(out);
    }

private:
    bool use_globals;
    bool had_error = false;
    int32_t cev_index = 0;
    int32_t cdb_index = 0;
    size_t curr_label_name = 0;
    GameData gd;

    size_t new_label() {
        return curr_label_name++;
    }

    enum WolfType {
        WT_NUMLIT,
        WT_NUMREF,
        WT_STRREF,
        WT_STRLIT,
        WT_DB
    };

    struct WolfValue {
        WolfType wt;
        int32_t v;
        std::string string_lit;
        int32_t db_type;
        int32_t db_data;
        int32_t db_prop;
        WolfValue() = default;
        WolfValue(std::string s) : wt(WT_STRLIT), string_lit(s) {}
        WolfValue(WolfType wt, int32_t v) : wt(wt), v(v) {}
        WolfValue(int32_t db_type, int32_t db_data, int32_t db_prop)
            : wt(WT_DB)
            , db_type(db_type), db_data(db_data), db_prop(db_prop) {}
        bool is_ref() { return wt == WT_NUMREF || wt == WT_STRREF; }
        bool do_suppress() { return wt == WT_NUMLIT && v >= REF_THRESHOLD; }
    };

    struct InlineFrame {
        FunctionStmt* fn;
        WolfValue ret;
        std::vector<WolfValue> args;
        size_t exit_label;
        InlineFrame(FunctionStmt* fn, WolfValue ret, std::vector<WolfValue> args, size_t exit_label)
        : fn(fn), ret(ret), args(args), exit_label(exit_label) {}
    };
    std::stack<InlineFrame> inline_funcs;

    CommonEvent* current_cev = nullptr;
    int32_t BASE_INT_SP;
    int32_t BASE_STR_SP;
    int32_t MAX_INT_SP;
    int32_t MAX_STR_SP;
    int32_t int_sp;
    int32_t str_sp;

    struct VarScope {
        const int32_t int_bp;
        const int32_t str_bp;
    };
    std::stack<VarScope> scopes;
    void begin_frame() { scopes.push({int_sp, str_sp}); };
    void end_frame() { int_sp = scopes.top().int_bp; str_sp = scopes.top().str_bp; scopes.pop(); };

    WolfValue eval(Expr* expr) { expr->accept(this); return expr_return; }
    WolfValue expr_return;

    bool try_const(Expr* expr) {
        if (expr->is_const) {
            if (expr->type == TYPE_INT)
                expr_return = WolfValue(WT_NUMLIT, expr->const_int);
            else
                expr_return = WolfValue(expr->const_str);
            return true;
        }
        return false;
    }

    WolfValue push_int() {
        if (++int_sp == CSELF_THRESHOLD + 5) int_sp = CSELF_THRESHOLD + 10;
        if (int_sp > MAX_INT_SP) error("Integer stack overflow");
        return WolfValue(WT_NUMREF, int_sp);
    }

    WolfValue push_str() {
        if (++str_sp > MAX_STR_SP) error("String stack overflow");
        return WolfValue(WT_STRREF, str_sp);
    }

    WolfValue try_suppress(WolfValue v) {
        if (!v.do_suppress()) return v;
        WolfValue temp = push_int();
        cmd_arith(temp, v);
        return temp;
    }

    bool update_prev_assign(WolfValue lhs) {
        Command& back = current_cev->commands.back();
        switch (back.command_id) {
            case CMD_ARITH:
                back.int_fields.at(0) = lhs.v;
                break;
            case CMD_DB:
                assert(back.int_fields.at(3) & DB_ASSIGN_TO_VAR);
                back.int_fields.at(4) = lhs.v;
                break;
            case CMD_STRING:
                back.int_fields.at(0) = lhs.v;
                break;
            case CMD_CALL_ID:
                back.int_fields.back() = lhs.v;
                break;
            case CMD_LOOP_END: // inline function return
                return false;
            default:
                assert(false);
        }
        return true;
    }

    void cmd_arith(WolfValue lhs, int32_t rhs_0) {
        cmd_arith(lhs, WolfValue(WT_NUMLIT, rhs_0), WolfValue(WT_NUMLIT, 0), ARITH_OP_PLUS);
    }

    void cmd_arith(WolfValue lhs, WolfValue rhs_0) {
        cmd_arith(lhs, rhs_0, WolfValue(WT_NUMLIT, 0), ARITH_OP_PLUS);
    }

    void cmd_arith(WolfValue lhs, WolfValue rhs_0, WolfValue rhs_1, ArithFlag flag) {
        assert(lhs.wt = WT_NUMREF);
        assert(v_is_cself(lhs.v) || v_is_normalvar(lhs.v));
        assert(rhs_0.wt == WT_NUMLIT || v_is_cself(rhs_0.v) || v_is_normalvar(rhs_0.v));
        assert(rhs_1.wt == WT_NUMLIT || v_is_cself(rhs_1.v) || v_is_normalvar(rhs_1.v));
        // if a rhs value should be suppressed, do so by assigning (0 - -rhs.v) into a temporary
        // this is generally better than using the built-in suppression flag for two reasons:
        // 1. it seems to run faster
        // 2. the engine normally clamps literals greater than 2 billion to +2,000,000,000, but doing this
        //    roundabout assignment avoids that
        // the cons for doing this involve using more temporaries and using more commands
        begin_frame();
        if (rhs_0.do_suppress()) {
            WolfValue tmp = push_int();
            cmd_arith(tmp, WolfValue(WT_NUMLIT, 0), WolfValue(WT_NUMLIT, -rhs_0.v), ARITH_OP_MINUS);
            rhs_0 = tmp;
        }
        if (rhs_1.do_suppress()) {
            WolfValue tmp = push_int();
            cmd_arith(tmp, WolfValue(WT_NUMLIT, 0), WolfValue(WT_NUMLIT, -rhs_1.v), ARITH_OP_MINUS);
            rhs_1 = tmp;
        }
        end_frame();
        current_cev->add_cmd(CMD_ARITH, {lhs.v, rhs_0.v, rhs_1.v, flag}, {});
    }

    void cmd_string(WolfValue lhs, WolfValue rhs) {
        cmd_string(lhs, rhs, STRING_ASSIGN_EQ);
    }

    void cmd_string(WolfValue lhs, WolfValue rhs, StringFlag flag) {
        assert(lhs.wt = WT_STRREF);
        if (rhs.wt == WT_STRLIT)
            current_cev->add_cmd(CMD_STRING, {lhs.v, STRING_RHS_LIT | flag, 0}, {'"' + rhs.string_lit + '"'});
        else if (rhs.wt == WT_STRREF)
            current_cev->add_cmd(CMD_STRING, {lhs.v, STRING_RHS_REF | flag, rhs.v}, {});
        else assert(false);
    }

    // eval an expression and put it into a cself temporary if the result was cdb
    WolfValue try_cdb_eval(Expr* expr) {
        WolfValue res = eval(expr);
        assert(expr->type != TYPE_INTARR);
        if (res.wt == WT_DB) {
            WolfValue new_res;
            if (expr->type == TYPE_INT)
                new_res = push_int();               
            else
                new_res = push_str();
            cmd_cdb_get(new_res, res, DB_ASSIGN_EQ);
            res = new_res;
        }
        return res;
    }

    void cmd_cdb_get(WolfValue lhs, WolfValue rhs, DBFlag flags) {
        assert(lhs.wt != WT_DB && rhs.wt == WT_DB);
        current_cev->add_cmd(CMD_DB, 
            {rhs.db_type, rhs.db_data, rhs.db_prop, 
                DB_TYPE_CDB | DB_ASSIGN_TO_VAR | flags, lhs.v}, 
            {"", "", "", ""});
    }

    void cmd_cdb_put(WolfValue lhs, WolfValue rhs, DBFlag flags) {
        assert(lhs.wt == WT_DB && rhs.wt != WT_DB);
        if (rhs.do_suppress()) {
            begin_frame();
            WolfValue temp_rhs = push_int();
            end_frame();
            cmd_arith(temp_rhs, rhs);
            rhs = temp_rhs;
        }
        if (rhs.wt == WT_STRLIT) {
            current_cev->add_cmd(CMD_DB, 
                {lhs.db_type, lhs.db_data, lhs.db_prop, 
                    DB_TYPE_CDB | DB_STRLIT | flags}, 
                {rhs.string_lit, "", "", ""});
        } else {
            current_cev->add_cmd(CMD_DB, 
                {lhs.db_type, lhs.db_data, lhs.db_prop, 
                    DB_TYPE_CDB | flags, rhs.v}, 
                {"", "", "", ""});
        }
    }

    void cmd_int_if(bool has_else, WolfValue left, WolfValue right, IfIntBranchFlag op) {
        assert((left.wt  == WT_NUMLIT || left.wt  == WT_NUMREF)
            && (right.wt == WT_NUMLIT || right.wt == WT_NUMREF));
        int32_t branch_flag =
            op | (right.do_suppress() ? IF_INT_BRANCH_SUPPRESS : 0);
        if (left.do_suppress()) {
            WolfValue new_left = push_int();
            cmd_arith(new_left, left);
            left = new_left;
        }
        current_cev->add_cmd(CMD_IF_INT,
            {1 | (has_else ? IF_HAS_ELSE : 0), left.v, right.v, branch_flag}, {});
        
        current_cev->add_cmd(CMD_BRANCH, {1}, {});
        current_cev->indent();
    }

    void cmd_str_if(bool has_else, WolfValue left, WolfValue right, IfStrBranchFlag op) {
        assert((left.wt  == WT_STRLIT || left.wt  == WT_STRREF)
            && (right.wt == WT_STRLIT || right.wt == WT_STRREF));
        if (left.wt == WT_STRLIT) {
            WolfValue new_left = push_str();
            cmd_string(new_left, left);
            left = new_left;
        }
        if (right.wt == WT_STRREF)
            current_cev->add_cmd(CMD_IF_STR,
                {1 | (has_else ? IF_HAS_ELSE : 0), left.v | op | IF_STR_BRANCH_REF, right.v},
                {""});
        else
            current_cev->add_cmd(CMD_IF_STR,
                {1 | (has_else ? IF_HAS_ELSE : 0), left.v | op},
                {right.string_lit});
         
        current_cev->add_cmd(CMD_BRANCH, {1}, {});
        current_cev->indent();
    }

    void cmd_new_branch(int32_t branch_num) {
        assert(branch_num > 1);
        current_cev->outdent();
        current_cev->add_cmd(CMD_BRANCH, {branch_num}, {});
        current_cev->indent();
    }

    void cmd_else() {
        current_cev->outdent();
        current_cev->add_cmd(CMD_BRANCH_ELSE, {0}, {});
        current_cev->indent();
    }

    void cmd_end_if() {
        current_cev->outdent();
        current_cev->add_cmd(CMD_IF_END);
    }

    std::tuple<int32_t, int32_t, int32_t> get_db_parts(int32_t ref) {
        int32_t trunc = ref % 100000000;
        int32_t type = trunc / 1000000;
        int32_t data = trunc % 1000000 / 100;
        int32_t prop = trunc % 100;
        return {type, data, prop};
    }

    bool v_is_cself(int32_t v) { return v >= 1600000 && v <= 1600099; }
    bool v_is_normalvar(int32_t v) { return v >= 2000000 && v < 2100000; }

    // error
    void error(std::string error_msg) {
        had_error = true;
        std::cout << "code generation error: " << error_msg << std::endl;
        std::cout << "inline stack trace: " << std::endl;
        while (!inline_funcs.empty()) {
            std::cout << inline_funcs.top().fn->name << std::endl;
            inline_funcs.pop();
        }
        throw std::runtime_error(error_msg);
    }
};