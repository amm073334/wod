#pragma once

#include <iostream>
#include <stack>
#include "visitor.h"
#include "ast.h"
#include "commonevent.h"
#include "command.h"

class Codegen : public Visitor {
public:
    std::string gen(std::vector<Stmt*> &program) {
        for (Stmt* s : program) {
            s->accept(this);
        }

        std::string out =
            "[COMMON_EVENT_TEXT_OUTPUT]\nCOMMON_EVENT_NUM=" + std::to_string(cevs.size()) + "\n";
        for (CommonEvent &cev : cevs) {
            out += "--------------------------\n";
            out += cev.to_string();
        }
        return out;
    }

    bool failed() { return had_error; }

    // statements
    void visit_FunctionStmt(FunctionStmt* stmt) override {
        if (stmt->is_inline) return;

        str_sp = CSELF_THRESHOLD + 5;
        for (WodType t : stmt->sym->arg_types)
            if (t == TYPE_STR) str_sp++;

        begin_frame();

        cevs.push_back(CommonEvent());
        current_cev = &cevs.back();

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
        if (current_inline_function) inline_return(stmt);
        else normal_return(stmt);
    }

    void normal_return(ReturnStmt* stmt) {
        if (!stmt->expr) {
            current_cev->add_cmd(CMD_RETURN);
            return;
        }
        
        begin_frame();
        WolfValue v = eval(stmt->expr);
        if (current_cev->RETURN_VAL_TARGET == -1)
            current_cev->RETURN_VAL_TARGET = 99;
        switch (stmt->expr->type) {
            case TYPE_INT:
                cmd_arith(WolfValue{WT_NUMREF, CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET}, v);
                break;
            case TYPE_STR:
                cmd_string(WolfValue{WT_STRREF, CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET}, v);
                break;
        }
        current_cev->add_cmd(CMD_RETURN);
        end_frame();
    }

    void inline_return(ReturnStmt* stmt) {
        if (!stmt->expr) {
            current_cev->add_cmd(CMD_BREAK);
            return;
        }
        begin_frame();
        WolfValue v = eval(stmt->expr);
        switch (stmt->expr->type) {
            case TYPE_INT:
                cmd_arith(inline_retval, v);
                break;
            case TYPE_STR:
                cmd_string(inline_retval, v);
                break;
        }
        current_cev->add_cmd(CMD_BREAK);
        end_frame();
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        begin_frame();
        eval(stmt->expr);
        end_frame();
    }

    void visit_VarStmt(VarStmt* stmt) override {
        if (stmt->is_const) return;

        WolfValue lhs;
        if (stmt->sym->type == TYPE_INT)
            lhs = push_int();
        else
            lhs = push_str();
        stmt->sym->ref = lhs.v;

        if (!stmt->initializer) return;

        begin_frame();
        WolfValue initial = eval(stmt->initializer);
        if (stmt->sym->type == TYPE_INT)
            cmd_arith(lhs, initial);
        else
            cmd_string(lhs, initial);
        end_frame();
    }

    void visit_IfStmt(IfStmt* stmt) override {
        begin_frame();
        WolfValue cond = eval(stmt->condition);
        if (stmt->else_branch)
            cmd_int_if(true, cond, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
        else
            cmd_int_if(false, cond, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
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
            WolfValue count = try_suppress(eval(stmt->count));
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
            int_fields.push_back(try_suppress(eval(e)).v);
        for (Expr* e: stmt->str_fields)
            str_fields.push_back(e->const_str);

        current_cev->add_cmd(cmd_id, int_fields, str_fields);
        end_frame();
    }

    // expressions
    void visit_AssignExpr(AssignExpr* expr) override {
        WolfValue lhs = eval(expr->lhs);
        WolfValue rhs = eval(expr->rhs);
        
        if (lhs.wt == WT_NUMREF)
            cmd_arith(lhs, rhs);
        else
            cmd_string(lhs, rhs);
    }

    void visit_VariableExpr(VariableExpr* expr) override {
        if (try_const(expr)) return;
        if (current_inline_function) inline_var(expr);
        else normal_var(expr);
    }

    void normal_var(VariableExpr* expr) {
        Symbol* sym = expr->sym;
        if (sym->type == TYPE_INT)
            expr_return = WolfValue{WT_NUMREF, sym->ref};
        else
            expr_return = WolfValue{WT_STRREF, sym->ref};
    }

    void inline_var(VariableExpr* expr) {
        for (size_t i = 0; i < current_inline_function->params.size(); i++) {
            if (current_inline_function->params.at(i)->sym == expr->sym) {
                expr_return = inline_args.at(i);
                return;
            }
        }
        normal_var(expr);
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
        WolfValue left = eval(expr->left);
        end_frame();

        begin_frame();
        WolfValue right;
        switch (expr->op) {
            case BinaryExpr::LOGIC_AND:
                cmd_int_if(true, left, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    right = eval(expr->right);
                    cmd_int_if(true, right, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    cmd_arith(temp, 1);
                    cmd_else();
                    cmd_arith(temp, 0);
                    cmd_end_if();
                cmd_else();
                    cmd_arith(temp, 0);
                cmd_end_if();
                break;
            case BinaryExpr::LOGIC_OR:
                cmd_int_if(true, left, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    cmd_arith(temp, 1);
                cmd_else();
                    right = eval(expr->right);
                    cmd_int_if(true, right, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
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
        WolfValue left = eval(expr->left);
        WolfValue right = eval(expr->right);
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
        WolfValue left = eval(expr->left);
        WolfValue right = eval(expr->right);
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
                cmd_arith(new_right, WolfValue{WT_NUM, 0}, right, ARITH_OP_MINUS);
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
        WolfValue right = eval(expr->right);
        switch (expr->op) {
            case UnaryExpr::LOGIC_NOT:
                cmd_int_if(true, right, WolfValue{WT_NUM, 0}, IF_INT_OP_EQ);
                cmd_arith(temp, 1);
                cmd_else();
                cmd_arith(temp, 0);
                cmd_end_if();
                break;
            case UnaryExpr::MINUS:
                cmd_arith(temp, WolfValue{WT_NUM, 0}, right, ARITH_OP_MINUS);
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
        inline_retval = push_int();

        begin_frame();
        int32_t cev_ref = expr->sym->ref;
        for (Expr* arg : expr->args) {
            inline_args.push_back(eval(arg));
        }

        current_cev->add_cmd(CMD_LOOP_COUNT, {1}, {});
        current_cev->indent();
        current_inline_function = expr->sym->inline_function;
        for (Stmt* s : expr->sym->inline_function->body) {
            s->accept(this);
        }
        current_inline_function = nullptr;
        current_cev->outdent();
        current_cev->add_cmd(CMD_LOOP_END);
        end_frame();

        expr_return = inline_retval;
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
            WolfValue result = try_suppress(eval(arg));
            switch (result.wt) {
                case WT_NUM:
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
        expr_return = WolfValue{WT_NUM, expr->value};
    }
    
    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {
        expr_return = WolfValue{WT_STRLIT, 0, expr->value};
    }

private:
    bool had_error = false;
    FunctionStmt* current_inline_function = nullptr;
    
    enum WolfType {
        WT_NUM,
        WT_NUMREF,
        WT_STRREF,
        WT_STRLIT
    };

    struct WolfValue {
        WolfType wt;
        int32_t v;
        std::string string_lit;
        bool is_ref() { return wt == WT_NUMREF || wt == WT_STRREF; }
        bool do_suppress() { return wt == WT_NUM && v >= VAR_THRESHOLD; }
    };
    WolfValue inline_retval;
    std::vector<WolfValue> inline_args;

    std::vector<CommonEvent> cevs;
    CommonEvent* current_cev = nullptr;
    int32_t int_sp = CSELF_THRESHOLD + 10 - 1; // start pointers at one less than the minimum value
    int32_t str_sp = CSELF_THRESHOLD + 5  - 1; // because a push operation will start by incrementing them to the minimum

    struct VarScope {
        const int32_t int_bp;
        const int32_t str_bp;
    };
    std::stack<VarScope> scopes;
    void begin_frame() { scopes.push({int_sp, str_sp}); };
    void end_frame() { int_sp = scopes.top().int_bp; str_sp = scopes.top().str_bp; scopes.pop(); };
    WolfValue int_top() { return WolfValue{WT_NUMREF, int_sp}; }
    WolfValue str_top() { return WolfValue{WT_STRREF, str_sp}; }

    WolfValue eval(Expr* expr) { expr->accept(this); return expr_return; }
    WolfValue expr_return;

    bool try_const(Expr* expr) {
        if (expr->is_const) {
            if (expr->type == TYPE_INT)
                expr_return = WolfValue{WT_NUM, expr->const_int};
            else
                expr_return = WolfValue{WT_STRLIT, 0, expr->const_str};
            return true;
        }
        return false;
    }

    WolfValue push_int() {
        if (++int_sp > MAX_CSELF_REF) error("Integer stack overflow");
        return WolfValue{WT_NUMREF, int_sp};
    }

    WolfValue push_str() {
        if (++str_sp > CSELF_THRESHOLD + 9) error("String stack overflow");
        return WolfValue{WT_STRREF, int_sp};
    }

    WolfValue try_suppress(WolfValue v) {
        if (!v.do_suppress()) return v;
        WolfValue temp = push_int();
        cmd_arith(temp, v);
        return temp;
    }

    void cmd_arith(WolfValue lhs, int32_t rhs_0) {
        cmd_arith(lhs, WolfValue{WT_NUM, rhs_0}, WolfValue{WT_NUM, 0}, ARITH_OP_PLUS);
    }

    void cmd_arith(WolfValue lhs, WolfValue rhs_0) {
        cmd_arith(lhs, rhs_0, WolfValue{WT_NUM, 0}, ARITH_OP_PLUS);
    }

    void cmd_arith(WolfValue lhs, WolfValue rhs_0, WolfValue rhs_1, ArithFlag flag) {
        assert(lhs.wt = WT_NUMREF);
        int32_t flags = flag
            | (rhs_0.do_suppress() ? ARITH_SUPPRESS_RHS_0 : 0)
            | (rhs_1.do_suppress() ? ARITH_SUPPRESS_RHS_1 : 0);
        current_cev->add_cmd(CMD_ARITH, {lhs.v, rhs_0.v, rhs_1.v, flags}, {});
    }

    void cmd_string(WolfValue lhs, WolfValue rhs) {
        assert(lhs.wt = WT_STRREF);
        if (rhs.wt == WT_STRLIT)
            current_cev->add_cmd(CMD_STRING, {lhs.v, STRING_RHS_LIT | STRING_ASSIGN_EQ, 0}, {rhs.string_lit});
        else if (rhs.wt == WT_STRREF)
            current_cev->add_cmd(CMD_STRING, {lhs.v, STRING_RHS_REF | STRING_ASSIGN_EQ, rhs.v}, {});
        else assert(false);
    }

    void cmd_int_if(bool has_else, WolfValue left, WolfValue right, IfIntBranchFlag op) {
        assert((left.wt  == WT_NUM || left.wt  == WT_NUMREF)
            && (right.wt == WT_NUM || right.wt == WT_NUMREF));
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

    // error
    void error(std::string error_msg) {
        had_error = true;
        std::cout << "code generation error: " << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
};