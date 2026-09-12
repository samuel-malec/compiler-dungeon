#pragma once

#include <cassert>
#include <map>

#include "cfg_builder.hpp"
#include "../hir/hir.hpp"
#include "../sema/symbol_table.hpp"

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
                default: assert(false && "unexpected op kind in lowering to ir phase");
            }
            assert(false && "unknown op kind");
        }

        label_data gen_label() {
            return label_data{.id = lab_idx++};
        }

        uint32_t bump_val() {
            return val_idx++;
        }

        value *get_value(const type *ty) {
            auto val = std::make_unique<value>(bump_val(), ty, std::vector<instruction *>{});
            ir_fn.values.push_back(std::move(val));
            return ir_fn.values.back().get();
        }

        void add_instr(opcode op, value *result, std::vector<value *> operands, instruction::data_t data) {
            auto instr = std::make_unique<instruction>();
            if (result != nullptr)
                result->defining_instruction = instr.get();

            instr->op = op;
            instr->result = result;
            instr->operands = std::move(operands);

            for (auto *operand: instr->operands)
                operand->users.push_back(instr.get());

            instr->data = std::move(data);
            ir_fn.instructions.push_back(std::move(instr));
        }

        bool current_path_terminated() const {
            if (ir_fn.instructions.empty())
                return false;
            return is_terminator(ir_fn.instructions.back()->op);
        }

        void br_if_not_terminated(uint32_t target_label_id) {
            if (!current_path_terminated())
                add_instr(opcode::br, nullptr, {}, br_data{.branch_id = target_label_id});
        }

        value *gen_bconst(bool val) {
            value *result = get_value(sema.types.get_bool());
            add_instr(opcode::bconst, result, {}, bconst_data{.value = val});
            return result;
        }

        value *shortcircuit_and(const hir::expr::binary_data *bd) {
            value *res = get_value(sema.types.get_bool());
            add_instr(opcode::alloca, res, {}, {});

            label_data ok = gen_label();
            label_data nok = gen_label();
            label_data end = gen_label();

            value *left = lower_hir_expr(bd->lhs);
            add_instr(opcode::cond_br, nullptr, {left}, cond_br_data{.true_branch = ok.id, .false_branch = nok.id});

            add_instr(opcode::label, nullptr, {}, nok);
            value *_false = gen_bconst(false);
            add_instr(opcode::store, nullptr, {res, _false}, {});
            br_if_not_terminated(end.id);

            add_instr(opcode::label, nullptr, {}, ok);
            value *rhs = lower_hir_expr(bd->rhs);
            add_instr(opcode::store, nullptr, {res, rhs}, {});
            br_if_not_terminated(end.id);

            add_instr(opcode::label, nullptr, {}, end);
            value *result = get_value(sema.types.get_bool());
            add_instr(opcode::load, result, {res}, {});
            return result;
        }

        value *shortcircuit_or(const hir::expr::binary_data *bd) {
            value *res = get_value(sema.types.get_bool());
            add_instr(opcode::alloca, res, {}, {});

            label_data ok = gen_label();
            label_data nok = gen_label();
            label_data end = gen_label();

            value *left = lower_hir_expr(bd->lhs);
            add_instr(opcode::cond_br, nullptr, {left}, cond_br_data{.true_branch = ok.id, .false_branch = nok.id});

            add_instr(opcode::label, nullptr, {}, ok);
            value *_true = gen_bconst(true);
            add_instr(opcode::store, nullptr, {res, _true}, {});
            br_if_not_terminated(end.id);

            add_instr(opcode::label, nullptr, {}, nok);
            value *rhs = lower_hir_expr(bd->rhs);
            add_instr(opcode::store, nullptr, {res, rhs}, {});
            br_if_not_terminated(end.id);

            add_instr(opcode::label, nullptr, {}, end);
            value *result = get_value(sema.types.get_bool());
            add_instr(opcode::load, result, {res}, {});
            return result;
        }

        value *lower_hir_expr(hir::expr_id eid) {
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
            if (auto t = std::get_if<hir::expr::binary_data>(&e.data)) {
                if (t->op == AND) return shortcircuit_and(t);
                if (t->op == OR) return shortcircuit_or(t);

                value *result = get_value(e.ty);
                value *lhs = lower_hir_expr(t->lhs);
                value *rhs = lower_hir_expr(t->rhs);
                add_instr(from_opkind(t->op), result, {lhs, rhs}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::relational_data>(&e.data)) {
                value *result = get_value(e.ty);
                value *lhs = lower_hir_expr(t->lhs);
                value *rhs = lower_hir_expr(t->rhs);
                add_instr(from_opkind(t->op), result, {lhs, rhs}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::assign_data>(&e.data)) {
                value *rhs = lower_hir_expr(t->value);
                value *target = symbol_value.at(t->target.value);
                add_instr(opcode::store, nullptr, {target, rhs}, {});
                return rhs;
            }
            if (auto t = std::get_if<hir::expr::call_data>(&e.data)) {
                const auto &fn_sig = sema.functions.at(t->target.value);
                assert(t->args.size() == fn_sig.param_types.size() &&
                    "call arity mismatch survived semantic analysis into IR lowering");

                value *result = get_value(e.ty);
                std::vector<value *> args;
                args.reserve(t->args.size());
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
                if (res && tbody)
                    add_instr(opcode::store, nullptr, {res, tbody}, {});
                br_if_not_terminated(end_lab.id);

                // else branch
                add_instr(opcode::label, nullptr, {}, else_lab);
                if (t->else_body) {
                    value *ebody = lower_hir_expr(*t->else_body);
                    if (res && ebody)
                        add_instr(opcode::store, nullptr, {res, ebody}, {});
                }
                br_if_not_terminated(end_lab.id);

                add_instr(opcode::label, nullptr, {}, end_lab);
                if (!res)
                    return nullptr; // unit-typed if — nothing to load

                value *result = get_value(e.ty);
                add_instr(opcode::load, result, {res}, {});
                return result;
            }
            if (auto t = std::get_if<hir::expr::while_data>(&e.data)) {
                label_data head_lab = gen_label();
                label_data body_lab = gen_label();
                label_data end_lab = gen_label();
                jmp_table.push_back({.cont_lab_id = head_lab.id, .break_lab_id = end_lab.id});

                add_instr(opcode::label, nullptr, {}, head_lab);
                value *cond = lower_hir_expr(t->cond);
                add_instr(opcode::cond_br, nullptr, {cond},
                          cond_br_data{.true_branch = body_lab.id, .false_branch = end_lab.id});

                add_instr(opcode::label, nullptr, {}, body_lab);
                lower_hir_expr(t->body);
                br_if_not_terminated(head_lab.id);

                add_instr(opcode::label, nullptr, {}, end_lab);
                jmp_table.pop_back();
                return nullptr; // while is always unit-typed
            }
            if (auto t = std::get_if<hir::expr::loop_data>(&e.data)) {
                auto body_lab = gen_label();
                auto end_lab = gen_label();
                jmp_table.push_back({.cont_lab_id = body_lab.id, .break_lab_id = end_lab.id});

                add_instr(opcode::label, nullptr, {}, body_lab);
                lower_hir_expr(t->body);
                br_if_not_terminated(body_lab.id);

                add_instr(opcode::label, nullptr, {}, end_lab);
                jmp_table.pop_back();
                return nullptr; // loop's own type is unit unless/until `break value;` exists
            }
            if (auto t = std::get_if<hir::expr::block_data>(&e.data)) {
                for (auto &s: t->stmts)
                    lower_hir_stmt(s);
                if (t->trailing)
                    return lower_hir_expr(*t->trailing);
                return nullptr; // no trailing expr => unit
            }

            assert(false && "unknown hir expression");
        }

        void lower_hir_stmt(hir::stmt_id sid) {
            auto &s = hir_fn.get_stmt(sid.idx);

            if (auto t = std::get_if<hir::stmt::expr_data>(&s.data)) {
                lower_hir_expr(t->e); // value (if any) is discarded, this is a statement
                return;
            }
            if (auto t = std::get_if<hir::stmt::let_data>(&s.data)) {
                value *res = get_value(hir_fn.get_expr(t->value.idx).ty);
                add_instr(opcode::alloca, res, {}, {});
                value *rhs = lower_hir_expr(t->value);
                if (rhs)
                    add_instr(opcode::store, nullptr, {res, rhs}, {});
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
                add_instr(opcode::br, nullptr, {}, br_data{.branch_id = jmp_table.back().break_lab_id});
                return;
            }
            if (std::get_if<hir::stmt::cont>(&s.data)) {
                add_instr(opcode::br, nullptr, {}, br_data{.branch_id = jmp_table.back().cont_lab_id});
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
        ir::module res{};
        for (size_t i = 0; i < mod.functions.size(); ++i) {
            const auto &function = mod.functions[i];
            ir_builder builder{function, sema};
            builder.ir_fn.return_type = sema.functions.at(i).return_type;
            res.funcs.push_back(builder.build());
        }
        return res;
    }
}
