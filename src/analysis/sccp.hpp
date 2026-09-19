#pragma once
#include "pass.hpp"

namespace dungeon {
    struct sccp : pass {
        struct overdefined {
        };

        struct unknown {
        };

        struct icons {
            uint64_t value;
        };

        struct bcons {
            bool value;
        };

        using constant = std::variant<icons, bcons>;

        struct lattice_element {
            using data_t = std::variant<unknown, overdefined, constant>;
            data_t data;

            constant as_constant() const {
                auto t = std::get_if<constant>(&data);
                assert(t);
                return *t;
            }

            bool is_top() const {
                return std::holds_alternative<overdefined>(data);
            }

            bool is_bot() const {
                return std::holds_alternative<unknown>(data);
            }

            bool is_constant() const {
                return std::holds_alternative<constant>(data);
            }
        };

        static lattice_element bot() {
            return {.data = unknown{}};
        }

        static lattice_element top() {
            return {.data = overdefined{}};
        }

        static lattice_element join(const lattice_element &lhs, const lattice_element &rhs) {
            if (lhs.is_top() || rhs.is_top()) {
                return top();
            }

            if (lhs.is_bot())
                return rhs;
            if (rhs.is_bot())
                return lhs;

            constant lcons = lhs.as_constant();
            constant rcons = rhs.as_constant();
            if (lcons.index() != rcons.index())
                return top();

            auto blhs = std::get_if<bcons>(&lcons);
            if (blhs) {
                auto brhs = std::get_if<bcons>(&rcons);
                return blhs->value == brhs->value ? lhs : top();
            }

            auto ilhs = std::get_if<icons>(&lcons);
            auto irhs = std::get_if<icons>(&rcons);
            return ilhs->value == irhs->value ? lhs : top();
        }

        using env = std::map<const ir::value *, lattice_element>;

        lattice_element get_element(const ir::value *v, const env &e) {
            if (!v || !v->defining_instruction) return {unknown{}};
            if (v->defining_instruction->op == ir::opcode::iconst)
                return {icons{std::get<ir::iconst_data>(v->defining_instruction->data).value}};
            if (v->defining_instruction->op == ir::opcode::bconst)
                return {bcons{std::get<ir::bconst_data>(v->defining_instruction->data).value}};
            if (auto it = e.find(v); it != e.end())
                return it->second;
            return {overdefined{}};
        }

        static bool is_foldable(ir::opcode op) {
            return op == ir::opcode::add || op == ir::opcode::sub ||
                   op == ir::opcode::mul || op == ir::opcode::div ||
                   op == ir::opcode::mod || op == ir::opcode::shl ||
                   op == ir::opcode::shr || op == ir::opcode::neg ||
                   op == ir::opcode::lnot || op == ir::opcode::lt ||
                   op == ir::opcode::eq;
        }

        lattice_element folded_value(ir::instruction *inst, const env &e) {
            auto lhs = get_element(inst->operands[0], e);
            if (!lhs.is_constant())
                return lhs;

            auto lhs_const = lhs.as_constant();
            if (inst->op == ir::opcode::neg)
                return {icons{.value = -std::get<icons>(lhs_const).value}};

            if (inst->op == ir::opcode::lnot)
                return {bcons{.value = !std::get<bcons>(lhs_const).value}};

            auto rhs = get_element(inst->operands[1], e);
            if (!rhs.is_constant())
                return {overdefined{}};

            auto rhs_const = rhs.as_constant();

            if (inst->op == ir::opcode::add)
                return {icons{std::get<icons>(lhs_const).value + std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::sub)
                return {icons{std::get<icons>(lhs_const).value - std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::mul)
                return {icons{std::get<icons>(lhs_const).value * std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::div) {
                // Hmm, should we crash the compiler if we encounter compile-time division by zero ?
                if (std::get<icons>(rhs_const).value == 0)
                    return {overdefined{}};
                return {icons{std::get<icons>(lhs_const).value / std::get<icons>(rhs_const).value}};
            }
            if (inst->op == ir::opcode::mod) {
                if (std::get<icons>(rhs_const).value == 0)
                    return {overdefined{}};
                return {icons{std::get<icons>(lhs_const).value % std::get<icons>(rhs_const).value}};
            }
            if (inst->op == ir::opcode::shl)
                return {icons{std::get<icons>(lhs_const).value << std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::shr)
                return {icons{std::get<icons>(lhs_const).value >> std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::eq) {
                auto blhs = std::get_if<bcons>(&lhs_const);
                auto brhs = std::get_if<bcons>(&rhs_const);
                if ((blhs && !brhs) || (!blhs && brhs)) {
                    // this should never happen in our type system, we will think if we want to change this later
                    assert(false && "should not reach here");
                }
                if (blhs && brhs)
                    return {bcons{.value = blhs->value == brhs->value}};

                auto ilhs = std::get_if<icons>(&lhs_const);
                auto irhs = std::get_if<icons>(&rhs_const);
                if ((ilhs && !irhs) || (!ilhs && irhs)) {
                    // this should never happen in our type system, we will think if we want to change this later
                    assert(false && "should not reach here");
                }

                assert(ilhs && irhs);
                return {bcons{.value = ilhs->value == irhs->value}};
            }
            if (inst->op == ir::opcode::lt) {
                auto ilhs = std::get_if<icons>(&lhs_const);
                auto irhs = std::get_if<icons>(&lhs_const);
                assert(ilhs && irhs);
                return {bcons{.value = ilhs->value < irhs->value}};
            }

            assert(false && "should not reach here");
        }

        void run(ir::function &fn) override {
            env e{};
            std::queue<basic_block *> flow_worklist{};
            std::queue<ir::instruction *> ssa_worklist{};
            flow_worklist.push(fn.entry);

            while (!flow_worklist.empty() || !ssa_worklist.empty()) {
                if (!flow_worklist.empty()) {
                    basic_block *current = flow_worklist.front();
                    flow_worklist.pop();

                    for (auto &ins: current->instructions) {
                        // ins->operands
                        if (ins->op == ir::opcode::phi) {
                            lattice_element curr = bot();
                            // We are abusing the fact that default constructed lattice element is bot in our case
                            for (auto &operand: ins->operands)
                                curr = join(curr, e[operand]);
                            e[ins->result] = curr;
                        } else if (ins->op == ir::opcode::br) {
                            flow_worklist.push(current->succ.front());
                        } else if (ins->op == ir::opcode::cond_br) {
                            lattice_element cond = e[ins->operands.front()];
                            if (!cond.is_constant()) {
                                flow_worklist.push(current->succ.front());
                                flow_worklist.push(current->succ.back());
                            } else if (std::get<bcons>(cond.as_constant()).value) {
                                // only true edge
                                flow_worklist.push(current->succ.front());
                            } else {
                                flow_worklist.push(current->succ.back());
                            }
                        } else {
                            lattice_element elem = folded_value(ins, e);
                            e[ins->result] = elem;
                        }
                    }
                }
            }
        }

        std::string description() override {
            return "Sparse Conditional Constant Propagation";
        }
    };
}
