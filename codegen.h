#pragma once

#include <iostream>
#include <stack>
#include "visitor.h"
#include "ast.h"
#include "commonevent.h"
#include "command.h"

class Codegen : Visitor {
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
        open_scope();

        cevs.push_back(CommonEvent());
        current_cev = &cevs.back();

        for (Stmt* s : stmt->body) {
            s->accept(this);
        }

        if (current_cev->COMMAND_NUM == 0) {
            current_cev->add_cmd(CMD_EMPTY);
        }

        close_scope();
    }

    void visit_BlockStmt(BlockStmt* stmt) override {
        open_scope();
        for (Stmt* s : stmt->stmts) {
            s->accept(this);
        }
        close_scope();
    }

    void visit_ReturnStmt(ReturnStmt* stmt) override {
        if (!stmt->expr) {
            current_cev->add_cmd(CMD_RETURN);
            return;
        }
        stmt->expr->accept(this);
        switch (stmt->expr->type) {
            case TYPE_INT:
                if (current_cev->RETURN_VAL_TARGET == -1)
                    current_cev->RETURN_VAL_TARGET = int_sp - CSELF_THRESHOLD;
                cmd_arith(WolfValue{WT_NUMREF, CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET}, eval_pop());
                break;
            case TYPE_STR:
                current_cev->RETURN_VAL_TARGET = str_sp - CSELF_THRESHOLD;
                cmd_string(WolfValue{WT_STRREF, CSELF_THRESHOLD + current_cev->RETURN_VAL_TARGET}, eval_pop());
                break;
        }
        current_cev->add_cmd(CMD_RETURN);
    }

    void visit_ExprStmt(ExprStmt* stmt) override {
        stmt->expr->accept(this);
        eval_pop();
    }

    void visit_VarStmt(VarStmt* stmt) override {
        WolfValue lhs;
        stmt->initializer->accept(this);
        if (stmt->sym->type == TYPE_INT) {
            lhs = new_int_var();
            cmd_arith(lhs, eval_pop());
        } else {
            lhs = new_str_var();
            cmd_string(lhs, eval_pop());
        }
        stmt->sym->ref = lhs.v;
    }

    void visit_AssignStmt(AssignStmt* stmt) override {
        Symbol* sym = stmt->env->get(stmt->name->text);
        if (sym->type == TYPE_INT)
            cmd_arith(WolfValue{WT_NUMREF, sym->ref}, eval_pop());
        else
            cmd_string(WolfValue{WT_STRREF, sym->ref}, eval_pop());
    }

    void visit_IfStmt(IfStmt* stmt) override {
        stmt->condition->accept(this);
        WolfValue cond = eval_pop();
        if (stmt->else_branch)
            cmd_int_if(true, cond, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
        else
            cmd_int_if(false, cond, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
        open_scope();
        stmt->then_branch->accept(this);
        close_scope();
        if (stmt->else_branch) {
            open_scope();
            stmt->else_branch->accept(this);
            close_scope();
        }
    }

    void visit_LoopStmt(LoopStmt* stmt) override {
        if (stmt->count) {
            stmt->count->accept(this);
            WolfValue count = eval_pop();
            if (count.suppress()) {
                cmd_arith(new_int_temp(), count);
                count = eval_pop();
            }
            current_cev->add_cmd(CMD_LOOP_COUNT, {count.v}, {});
        } else {
            current_cev->add_cmd(CMD_LOOP, {}, {});
        }
        open_scope();
        stmt->body->accept(this);
        close_scope();
        current_cev->add_cmd(CMD_LOOP_END, {}, {});
    }

    // expressions
    void visit_VariableExpr(VariableExpr* expr) override {
        Symbol* sym = expr->env->get(expr->name->text);
        if (sym->type == TYPE_INT)
            eval_push({WT_NUMREF, sym->ref});
        else
            eval_push({WT_STRREF, sym->ref});
    }

    void visit_BinaryExpr(BinaryExpr* expr) override {
        switch (expr->op->token_type) {
            case T_PIPE_PIPE:
            case T_AMP_AMP:
                binary_logical_expr(expr);
                break;
            case T_GREATER:
            case T_GREATER_EQUAL:
            case T_LESS:
            case T_LESS_EQUAL:
                binary_comp_expr(expr);
                break;
            default:
                binary_normal_expr(expr);
                break;
        }
    }

    void binary_logical_expr(BinaryExpr* expr) {
        expr->left->accept(this);
        WolfValue left = eval_pop();
        WolfValue right;
        WolfValue out = new_int_temp();
        switch (expr->op->token_type) {
            case T_AMP_AMP:
                cmd_int_if(true, left, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    expr->right->accept(this);
                    right = eval_pop();
                    cmd_int_if(true, right, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    cmd_arith(out, 1);
                    cmd_else();
                    cmd_arith(out, 0);
                    cmd_end_if();
                cmd_else();
                    cmd_arith(out, 0);
                cmd_end_if();
                break;
            case T_PIPE_PIPE:
                cmd_int_if(true, left, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    cmd_arith(out, 1);
                cmd_else();
                    expr->right->accept(this);
                    right = eval_pop();
                    cmd_int_if(true, right, WolfValue{WT_NUM, 0}, IF_INT_OP_NEQ);
                    cmd_arith(out, 1);
                    cmd_else();
                    cmd_arith(out, 0);
                    cmd_end_if();
                cmd_end_if();
                break;
        }
    }

    void binary_comp_expr(BinaryExpr* expr) {
        expr->left->accept(this);
        expr->right->accept(this);
        IfIntBranchFlag op;
        switch (expr->op->token_type) {
            case T_GREATER:         op = IF_INT_OP_GT; break;
            case T_GREATER_EQUAL:   op = IF_INT_OP_GTE; break;
            case T_LESS:            op = IF_INT_OP_LT; break;
            case T_LESS_EQUAL:      op = IF_INT_OP_LTE; break;
        }
        WolfValue right = eval_pop();
        WolfValue left = eval_pop();
        WolfValue out = new_int_temp();
        cmd_int_if(true, left, right, op);
        cmd_arith(out, 1);
        cmd_else();
        cmd_arith(out, 0);
        cmd_end_if();
    }

    void binary_normal_expr(BinaryExpr* expr) {
        expr->left->accept(this);
        expr->right->accept(this);
        WolfValue right = eval_pop();
        WolfValue left = eval_pop();
        ArithFlag op;
        switch (expr->op->token_type) {
            case T_PLUS:        op = ARITH_OP_PLUS; break;
            case T_MINUS:       op = ARITH_OP_MINUS; break;
            case T_STAR:        op = ARITH_OP_TIMES; break;
            case T_SLASH:       op = ARITH_OP_DIV; break;
            case T_PIPE:        op = ARITH_OP_OR; break;
            case T_AMP:         op = ARITH_OP_AND; break;
            case T_LESS_LESS:   op = ARITH_OP_LSHIFT; break;
            case T_GREATER_GREATER:
                op = ARITH_OP_LSHIFT;
                cmd_arith(new_int_temp(), WolfValue{WT_NUM, 0}, right, ARITH_OP_MINUS);
                right = eval_pop();
                break;
        }
        cmd_arith(new_int_temp(), left, right, op);
    }

    void visit_UnaryExpr(UnaryExpr* expr) override {
        expr->right->accept(this);
        WolfValue right = eval_pop();
        WolfValue out = new_int_temp();
        switch (expr->op->token_type) {
            case T_BANG:
                cmd_int_if(true, right, WolfValue{WT_NUM, 0}, IF_INT_OP_EQ);
                cmd_arith(out, 1);
                cmd_else();
                cmd_arith(out, 0);
                cmd_end_if();
                break;
            case T_MINUS:
                cmd_arith(out, WolfValue{WT_NUM, 0}, right, ARITH_OP_MINUS);
                break;
        }
    }

    void visit_CallExpr(CallExpr* expr) override {
        int32_t cev_ref = expr->env->get(expr->name->text)->ref;
        std::vector<int32_t> int_args;
        std::vector<int32_t> strref_args;
        std::vector<std::string> str_args = {""};
        int32_t n_int_args = 0;
        int32_t n_str_args = 0;
        int32_t strlit_flags = 0;
        for (Expr* arg : expr->args) {
            arg->accept(this);
            WolfValue result = eval_pop();
            switch (result.wt) {
                case WT_NUM:
                    if (result.v >= VAR_THRESHOLD)
                        cmd_arith(new_int_temp(), result);
                    int_args.push_back(eval_pop().v);
                    n_int_args++;
                    break;
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
        cmd_int_fields.push_back(new_int_temp().v);
        if (strlit_flags)
            current_cev->add_cmd(CMD_CALL_ID, cmd_int_fields, str_args);
        else
            current_cev->add_cmd(CMD_CALL_ID, cmd_int_fields, {});
    }

    void visit_IntLiteralExpr(IntLiteralExpr* expr) override {
        eval_push(WolfValue{WT_NUM, expr->value});
    }
    
    void visit_StrLiteralExpr(StrLiteralExpr* expr) override {
        eval_push(WolfValue{WT_STRLIT, 0, expr->value});
    }

private:
    bool had_error = false;
    
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
        bool suppress() { return v >= VAR_THRESHOLD && !is_ref(); }
    };

    std::vector<CommonEvent> cevs;
    CommonEvent* current_cev;
    int32_t int_sp = CSELF_THRESHOLD + 10;
    int32_t int_bp = CSELF_THRESHOLD + 10;
    int32_t str_sp = CSELF_THRESHOLD + 5;
    int32_t str_bp = CSELF_THRESHOLD + 5;

    struct VarScope {
        const int32_t int_bp;
        const int32_t str_bp;
    };
    std::stack<VarScope> scopes;
    void open_scope() { scopes.push({int_bp, str_bp}); };
    void close_scope() { int_sp = scopes.top().int_bp; str_sp = scopes.top().str_bp; scopes.pop(); };

    std::stack<WolfValue> eval_stack;
    void eval_push(WolfValue v) { eval_stack.push(v); }
    WolfValue eval_pop() { WolfValue tmp = eval_stack.top(); eval_stack.pop(); return tmp; }

    WolfValue new_int_temp() {
        if (++int_sp > MAX_CSELF_REF) error("Integer stack overflow");
        eval_push(WolfValue{WT_NUMREF, int_sp});
        return eval_stack.top();
    }

    WolfValue new_str_temp() {
        if (++str_sp > CSELF_THRESHOLD + 9) error("String stack overflow");
        eval_push(WolfValue{WT_STRREF, int_sp});
        return eval_stack.top();
    }

    WolfValue new_int_var() {
        if (++int_sp > MAX_CSELF_REF) error("Integer stack overflow");
        return WolfValue{WT_NUMREF, int_sp};
    }

    WolfValue new_str_var() {
        if (++int_sp > MAX_CSELF_REF) error("String stack overflow");
        return WolfValue{WT_STRREF, int_sp};
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
            | (rhs_0.suppress() ? ARITH_SUPPRESS_RHS_0 : 0)
            | (rhs_1.suppress() ? ARITH_SUPPRESS_RHS_1 : 0);
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
            op | (right.suppress() ? IF_INT_BRANCH_SUPPRESS : 0);
        if (left.suppress()) {
            cmd_arith(new_int_temp(), left);
            left = eval_pop();
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
            cmd_string(new_str_temp(), left);
            left = eval_pop();
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