#pragma once
#include <unordered_set>

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
                return std::holds_alternative<unknown>(data);
            }

            bool is_bot() const {
                return std::holds_alternative<overdefined>(data);
            }

            bool is_constant() const {
                return std::holds_alternative<constant>(data);
            }

            void meet_with(const lattice_element &o) {
                if (is_bot() || o.is_top())
                    return;
                if (o.is_bot()) {
                    data = bot().data;
                    return;
                }
                if (is_top()) {
                    data = o.data;
                    return;
                }

                assert(is_constant() && o.is_constant());
                constant lcons = as_constant();
                constant rcons = o.as_constant();
                if (lcons.index() != rcons.index()) {
                    data = bot().data;
                    return;
                }

                auto blhs = std::get_if<bcons>(&lcons);
                if (blhs) {
                    auto brhs = std::get_if<bcons>(&rcons);
                    if (blhs->value == brhs->value)
                        return;
                    data = bot().data;
                    return;
                }

                auto ilhs = std::get_if<icons>(&lcons);
                auto irhs = std::get_if<icons>(&rcons);
                if (ilhs->value == irhs->value)
                    return;
                data = bot().data;
            }

            bool operator==(const lattice_element &o) const {
                if (is_bot() && o.is_bot())
                    return true;
                if (is_top() && o.is_top())
                    return true;
                if (is_constant() && o.is_constant()) {
                    constant lcons = as_constant();
                    constant rcons = o.as_constant();
                    if (lcons.index() != rcons.index())
                        return false;

                    auto blhs = std::get_if<bcons>(&lcons);
                    if (blhs) {
                        auto brhs = std::get_if<bcons>(&rcons);
                        return blhs->value == brhs->value;
                    }

                    auto ilhs = std::get_if<icons>(&lcons);
                    auto irhs = std::get_if<icons>(&rcons);
                    return ilhs->value == irhs->value;
                }

                return false;
            }
        };

        static lattice_element bot() {
            return {.data = overdefined{}};
        }

        static lattice_element top() {
            return {.data = unknown{}};
        }

        using lattice = std::map<const ir::value *, lattice_element>;

        static bool is_foldable(ir::opcode op) {
            return op == ir::opcode::iconst || op == ir::opcode::bconst ||
                   op == ir::opcode::add || op == ir::opcode::sub ||
                   op == ir::opcode::mul || op == ir::opcode::div ||
                   op == ir::opcode::mod || op == ir::opcode::shl ||
                   op == ir::opcode::shr || op == ir::opcode::neg ||
                   op == ir::opcode::lnot || op == ir::opcode::lt ||
                   op == ir::opcode::eq;
        }

        static lattice_element folded_value(ir::instruction *inst, lattice &values) {
            if (inst->op == ir::opcode::iconst)
                return {icons{.value = std::get<ir::iconst_data>(inst->data).value}};

            if (inst->op == ir::opcode::bconst)
                return {bcons{.value = std::get<ir::bconst_data>(inst->data).value}};

            auto lhs = values[inst->operands[0]];
            if (!lhs.is_constant())
                return lhs;

            auto lhs_const = lhs.as_constant();
            if (inst->op == ir::opcode::neg)
                return {icons{.value = -std::get<icons>(lhs_const).value}};

            if (inst->op == ir::opcode::lnot)
                return {bcons{.value = !std::get<bcons>(lhs_const).value}};

            auto rhs = values[inst->operands[1]];
            if (rhs.is_top())
                return top();

            if (rhs.is_bot())
                return bot();

            assert(rhs.is_constant());
            auto rhs_const = rhs.as_constant();

            if (inst->op == ir::opcode::add)
                return {icons{std::get<icons>(lhs_const).value + std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::sub)
                return {icons{std::get<icons>(lhs_const).value - std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::mul)
                return {icons{std::get<icons>(lhs_const).value * std::get<icons>(rhs_const).value}};
            if (inst->op == ir::opcode::div) {
                // TODO: should we crash the compiler if we encounter compile-time division by zero ?
                if (std::get<icons>(rhs_const).value == 0)
                    return {overdefined{}};
                return {icons{std::get<icons>(lhs_const).value / std::get<icons>(rhs_const).value}};
            }
            if (inst->op == ir::opcode::mod) {
                // TODO: same question as in the div-case
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
                    return bot();
                }
                if (blhs && brhs)
                    return {bcons{.value = blhs->value == brhs->value}};

                auto ilhs = std::get_if<icons>(&lhs_const);
                auto irhs = std::get_if<icons>(&rhs_const);
                if ((ilhs && !irhs) || (!ilhs && irhs)) {
                    assert(false && "should not reach here");
                    return bot();
                }

                assert(ilhs && irhs);
                return {bcons{.value = ilhs->value == irhs->value}};
            }
            if (inst->op == ir::opcode::lt) {
                auto ilhs = std::get_if<icons>(&lhs_const);
                auto irhs = std::get_if<icons>(&rhs_const);
                assert(ilhs && irhs);
                return {bcons{.value = ilhs->value < irhs->value}};
            }

            assert(false && "should not reach here");
        }

        void mark_executable(edge e) {
            if (executable_blocks.insert(e.to).second)
                flow_worklist.push(e.to);
            if (executable_edges.insert(e).second) {
                for (auto ins: e.to->instructions) {
                    if (ins->op == ir::opcode::phi) {
                        ssa_worklist.push(ins);
                    }
                }
            }
        }

        void visit(ir::instruction *ins) {
            auto old = values[ins->result];
            if (ins->op == ir::opcode::br) {
                edge e{.from = ins->parent, .to = ins->parent->succ.front()};
                mark_executable(e);
                return;
            }

            if (ins->op == ir::opcode::cond_br) {
                edge true_edge{.from = ins->parent, .to = ins->parent->succ.front()};
                edge false_edge{.from = ins->parent, .to = ins->parent->succ.back()};

                lattice_element cond = values[ins->operands.front()];
                if (!cond.is_constant()) {
                    mark_executable(true_edge);
                    mark_executable(false_edge);
                } else if (std::get<bcons>(cond.as_constant()).value) {
                    mark_executable(true_edge);
                } else {
                    mark_executable(false_edge);
                }
                return;
            }

            if (ins->op == ir::opcode::phi) {
                auto pd = std::get<ir::phi_data>(ins->data);
                for (auto &[block, val]: pd.incoming) {
                    edge e{.from = block, .to = ins->parent};
                    if (executable_edges.contains(e))
                        values[ins->result].meet_with(values[val]);
                }
            } else if (is_foldable(ins->op))
                values[ins->result].meet_with(folded_value(ins, values));

            else if (ins->result)
                values[ins->result].meet_with(bot());

            if (old == values[ins->result])
                return;

            for (auto use: ins->result->users)
                ssa_worklist.push(use);
        }

        void initialize(ir::function &fn) {
            flow_worklist.push(fn.entry);
            executable_blocks.insert(fn.entry);

            while (!flow_worklist.empty() || !ssa_worklist.empty()) {
                if (!flow_worklist.empty()) {
                    basic_block *current = flow_worklist.front();
                    flow_worklist.pop();
                    for (auto &ins: current->instructions)
                        visit(ins);

                    if (!current->has_terminator() && !current->succ.empty())
                        mark_executable(edge{.from = current, .to = current->succ.front()});
                }
                if (!ssa_worklist.empty()) {
                    ir::instruction *ins = ssa_worklist.front();
                    ssa_worklist.pop();
                    visit(ins);
                }
            }
        }

        void transform(ir::function &fn) {
            for (auto &block: fn.blocks) {
                for (auto inst: block->instructions) {
                    // Convert cond_br with constant condition to br
                    if (inst->op == ir::opcode::cond_br) {
                        auto cond = inst->operands[0];
                        auto elem = values[cond];
                        if (!elem.is_constant())
                            continue;

                        auto cbr = std::get<ir::cond_br_data>(inst->data);
                        auto true_id = cbr.true_branch;
                        auto false_id = cbr.false_branch;
                        auto true_branch = block->succ[0];
                        auto false_branch = block->succ[1];
                        bool taken = std::get<bcons>(elem.as_constant()).value;

                        block->succ.clear();
                        inst->erase_operands();
                        inst->op = ir::opcode::br;
                        inst->data = ir::br_data{.branch_id = taken ? true_id : false_id};
                        block->succ.push_back(taken ? true_branch : false_branch);
                        continue;
                    }

                    // Propagate constants
                    auto elem = values[inst->result];
                    if (!elem.is_constant())
                        continue;

                    inst->erase_operands();
                    auto cons = elem.as_constant();

                    if (auto ic = std::get_if<icons>(&cons)) {
                        inst->op = ir::opcode::iconst;
                        inst->data = ir::iconst_data{.value = ic->value};
                    }
                    if (auto bc = std::get_if<bcons>(&cons)) {
                        inst->op = ir::opcode::bconst;
                        inst->data = ir::bconst_data{.value = bc->value};
                    }
                }
            }
        }

        void run(ir::function &fn) override {
            initialize(fn);
            transform(fn);
        }

        std::string description() override {
            return "Sparse Conditional Constant Propagation";
        }

        lattice values{};
        std::queue<basic_block *> flow_worklist{};
        std::unordered_set<basic_block *> executable_blocks{};
        std::set<edge> executable_edges{};
        std::queue<ir::instruction *> ssa_worklist{};
    };
}
