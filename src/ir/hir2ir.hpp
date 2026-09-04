#pragma once
#include "cfg_builder.hpp"
#include "../hir/hir.hpp"
#include <cassert>
#include <map>

// TODO: goals: create control flow representation, lazy evaluation of boolean expressions
namespace dungeon::ir {
    struct ir_builder {
        const hir::function &hir_fn;
        const sema::analysis_result &sema;

        function ir_fn{};
        uint32_t val_idx = 1;
        uint32_t lab_idx = 1;
        std::map<uint32_t, value *> symbol_value;

        struct jmp_data {
            uint32_t cont_lab_id;
            uint32_t break_lab_id;
        };

        std::vector<jmp_data> jmp_table;

        explicit ir_builder(const hir::function &hir_fn, const sema::analysis_result &sema)
            : hir_fn{hir_fn}, sema{sema} {
        }

        opcode from_opkind(op_kind op) {
            switch (op) {
                case ADD: return opcode::add;
                case SUB: return opcode::sub;
                case MUL: return opcode::mul;
                case DIV: return opcode::div;
                case MOD: return opcode::mod;
                case SHL: return opcode::shl;
                case SHR: return opcode::shr;
                case MINUS: return opcode::neg;
                case EQ: return opcode::eq;
                case LT: return opcode::lt;
                case NOT: return opcode::lnot;
                case AND: return opcode::land;
                case OR: return opcode::lor;
                default: assert(false && "unexpected op kind in lowering to ir phase");
            }
            assert(false && "unknown op kind");
        }

        label_data gen_label() {
            return label_data{.id = lab_idx++};
        }

        uint32_t bump_val() {
            return val_idx++;
        };

        value *get_value(const type *ty) {
            auto val = std::make_unique<value>(bump_val(), ty, std::vector<instruction *>{});
            ir_fn.values.push_back(std::move(val));
            return ir_fn.values.back().get();
        }

        void add_instr(opcode op, value *result, std::vector<value *> operands, instruction::data_t data) {
            instruction i{};
            ir_fn.instructions.push_back(std::make_unique<instruction>(std::move(i)));
            instruction *curr = ir_fn.instructions.back().get();

            curr->op = op;
            curr->result = result;
            curr->operands = std::move(operands);
            for (auto operand: curr->operands)
                operand->users.push_back(curr);
            curr->data = data;
        }

        bool current_path_terminated() const {
            if (ir_fn.instructions.empty())
                return false;
            const opcode op = ir_fn.instructions.back()->op;
            return is_terminator(op);
        }

        value *lower_hir_expr(hir::expr_id eid) {
            // TODO:: add instructions
            auto &e = hir_fn.get_expr(eid.idx);

            if (auto t = std::get_if<hir::expr::int_lit>(&e.data)) {
                value *result = get_value(e.ty);
                add_instr(opcode::iconst, result, {}, iconst_data{.value = t->val});
                return result;
            }
            if (auto t = std::get_if<hir::expr::bool_lit>(&e.data)) {
                value *result = get_value(e.ty);
                add_instr(opcode::bconst, result, {}, bconst_data{.value = t->val});
                return result;
            }
            if (auto t = std::get_if<hir::expr::var_data>(&e.data)) {
                value *result = get_value(e.ty);
                add_instr(opcode::load, result, {symbol_value.at(t->sid.value)}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::unary_data>(&e.data)) {
                value *result = get_value(e.ty);
                add_instr(from_opkind(t->op), result, {lower_hir_expr(t->lhs)}, {});
                return result;
            }
            // TODO: we should think about lazy evaluation of boolean expressions here
            if (auto t = std::get_if<hir::expr::binary_data>(&e.data)) {
                value *result = get_value(e.ty);
                value *lhs = lower_hir_expr(t->lhs);
                value *rhs = lower_hir_expr(t->rhs);
                auto opcode = from_opkind(t->op);
                add_instr(opcode, result, {lhs, rhs}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::relational_data>(&e.data)) {
                value *result = get_value(e.ty);
                value *lhs = lower_hir_expr(t->lhs);
                value *rhs = lower_hir_expr(t->rhs);
                auto opcode = from_opkind(t->op);
                add_instr(opcode, result, {lhs, rhs}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::assign_data>(&e.data)) {
                value *rhs = lower_hir_expr(t->value);
                value *target = symbol_value.at(t->target.value);
                add_instr(opcode::store, nullptr, {target, rhs}, {});
                return nullptr;
            }
            if (auto t = std::get_if<hir::expr::call_data>(&e.data)) {
                value *result = get_value(e.ty);
                std::vector<value *> args;
                for (auto &arg: t->args)
                    args.push_back(lower_hir_expr(arg));
                add_instr(opcode::call, result, std::move(args), call_data{.target = t->target});
                return result;
            }
            if (auto t = std::get_if<hir::expr::if_data>(&e.data)) {
                value *res = nullptr;
                if (!is_unit(e.ty)) {
                    res = get_value(e.ty);
                    add_instr(opcode::alloca, res, {}, {});
                }

                auto then_lab = gen_label();
                auto else_lab = gen_label();
                auto end_lab = gen_label();

                value *cond = lower_hir_expr(t->cond);
                add_instr(opcode::cond_br, nullptr, {cond}, cond_br_data{
                              .true_branch = then_lab.id, .false_branch = else_lab.id
                          });
                // then branch
                add_instr(opcode::label, nullptr, {}, then_lab);
                value *tbody = lower_hir_expr(t->then_body);
                if (res)
                    add_instr(opcode::store, {}, {res, tbody}, {});

                if (!current_path_terminated())
                    add_instr(opcode::br, nullptr, {}, br_data{.branch_id = end_lab.id});

                // else branch
                add_instr(opcode::label, nullptr, {}, else_lab);
                value *ebody = nullptr;
                if (t->else_body) {
                    ebody = lower_hir_expr(*t->else_body);
                    if (res)
                        add_instr(opcode::store, {}, {res, ebody}, {});
                    if (!current_path_terminated())
                        add_instr(opcode::br, nullptr, {}, br_data{.branch_id = end_lab.id});
                }
                add_instr(opcode::label, nullptr, {}, end_lab);
                if (!res)
                    return nullptr;

                value *result = get_value(e.ty);
                add_instr(opcode::load, result, {res}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::while_data>(&e.data)) {
                label_data head_lab = gen_label();
                label_data body_lab = gen_label();
                label_data end_lab = gen_label();
                jmp_data curr{.cont_lab_id = head_lab.id, .break_lab_id = end_lab.id};
                jmp_table.push_back(curr);

                add_instr(opcode::label, nullptr, {}, head_lab);
                value *cond = lower_hir_expr(t->cond);
                add_instr(opcode::cond_br, nullptr, {cond},
                          cond_br_data{.true_branch = body_lab.id, .false_branch = end_lab.id});
                add_instr(opcode::label, nullptr, {}, body_lab);
                lower_hir_expr(t->body);
                if (!current_path_terminated())
                    add_instr(opcode::br, nullptr, {}, br_data{.branch_id = head_lab.id});
                add_instr(opcode::label, nullptr, {}, end_lab);

                jmp_table.pop_back();
                return nullptr;
            }
            if (auto t = std::get_if<hir::expr::loop_data>(&e.data)) {
                auto body_lab = gen_label();
                auto end_lab = gen_label();

                jmp_data curr{.cont_lab_id = body_lab.id, .break_lab_id = end_lab.id};
                jmp_table.push_back(curr);
                add_instr(opcode::label, nullptr, {}, body_lab);
                lower_hir_expr(t->body);
                if (!current_path_terminated())
                    add_instr(opcode::br, nullptr, {}, br_data{.branch_id = body_lab.id});
                add_instr(opcode::label, nullptr, {}, end_lab);

                jmp_table.pop_back();
                return nullptr;
            }
            if (auto t = std::get_if<hir::expr::block_data>(&e.data)) {
                for (auto &s: t->stmts) {
                    lower_hir_stmt(s);
                }
                if (t->trailing)
                    return lower_hir_expr(*t->trailing);

                return nullptr;
            }

            assert(false && "unknown hir expression");
        }

        void lower_hir_stmt(hir::stmt_id sid) {
            auto &s = hir_fn.get_stmt(sid.idx);

            if (auto t = std::get_if<hir::stmt::expr_data>(&s.data)) {
                lower_hir_expr(t->e);
                return;
            }
            if (auto t = std::get_if<hir::stmt::let_data>(&s.data)) {
                value *res = get_value(hir_fn.get_expr(t->value.idx).ty);
                add_instr(opcode::alloca, res, {}, {});
                value *lhs = lower_hir_expr(t->value);
                if (lhs)
                    add_instr(opcode::store, nullptr, {res, lhs}, {});
                symbol_value[t->target.value] = res;
                return;
            }

            if (auto t = std::get_if<hir::stmt::ret_data>(&s.data)) {
                if (t->value)
                    add_instr(opcode::ret, nullptr, {lower_hir_expr(*t->value)}, {});
                else
                    add_instr(opcode::ret, nullptr, {}, {});
                return;
            }

            if (std::get_if<hir::stmt::brk>(&s.data)) {
                const jmp_data &curr = jmp_table.back();
                add_instr(opcode::br, nullptr, {}, br_data{.branch_id = curr.break_lab_id});
                return;
            }

            if (std::get_if<hir::stmt::cont>(&s.data)) {
                const jmp_data &curr = jmp_table.back();
                add_instr(opcode::br, nullptr, {}, br_data{.branch_id = curr.cont_lab_id});
                return;
            }
            assert(false && "unknown hir statement");
        }

        function build() {
            ir_fn.param_types.reserve(hir_fn.params.size());
            for (auto sid: hir_fn.params) {
                const auto &sym = sema.symbols.at(sid.value);
                const auto *var = std::get_if<sema::variable>(&sym.data);
                assert(var);
                ir_fn.param_types.push_back(var->ty);
                value *param = get_value(var->ty);
                add_instr(opcode::param, param, {}, param_data{
                              .index = static_cast<uint32_t>(ir_fn.param_types.size() - 1)
                          });
                symbol_value[sid.value] = param;
            }
            value *root = lower_hir_expr(hir_fn.root);
            if (root)
                add_instr(opcode::ret, nullptr, {root}, {});
            else if (ir_fn.instructions.empty() || !is_terminator(ir_fn.instructions.back()->op))
                add_instr(opcode::ret, nullptr, {}, {});
            return std::move(ir_fn);
        }
    };

    inline module lower_hir_to_ir(const hir::module &mod, const sema::analysis_result &sema) {
        module res{};
        for (size_t i = 0; i < mod.functions.size(); ++i) {
            const auto &function = mod.functions[i];
            ir_builder builder{function, sema};
            builder.ir_fn.return_type = sema.functions.at(i).return_type;
            res.funcs.push_back(builder.build());
        }

        return res;
    }
}
