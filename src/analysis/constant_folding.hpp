#pragma once
#include <iostream>

#include "pass.hpp"

namespace dungeon {
    struct constant_folding : pass {
        struct const_int {
            uint64_t value;
        };

        struct const_bool {
            bool value;
        };

        using const_value = std::variant<const_int, const_bool>;

        std::optional<const_value> as_constant(const ir::value *v) {
            if (!v || !v->defining_instruction) return std::nullopt;
            if (v->defining_instruction->op == ir::opcode::iconst)
                return const_int{std::get<ir::iconst_data>(v->defining_instruction->data).value};
            if (v->defining_instruction->op == ir::opcode::bconst)
                return const_bool{std::get<ir::bconst_data>(v->defining_instruction->data).value};
            return std::nullopt;
        }

        static bool is_foldable(ir::opcode op) {
            return op == ir::opcode::add || op == ir::opcode::sub ||
                   op == ir::opcode::mul || op == ir::opcode::div ||
                   op == ir::opcode::mod || op == ir::opcode::shl ||
                   op == ir::opcode::shr || op == ir::opcode::neg ||
                   op == ir::opcode::lnot;
        }

        std::optional<const_value> folded_value(ir::instruction *inst) {
            auto lhs = as_constant(inst->operands[0]);
            if (!lhs)
                return std::nullopt;

            if (inst->op == ir::opcode::neg)
                return const_int{.value = -std::get<const_int>(*lhs).value};

            if (inst->op == ir::opcode::lnot)
                return const_bool{.value = !std::get<const_bool>(*lhs).value};

            auto rhs = as_constant(inst->operands[1]);
            if (!rhs)
                return std::nullopt;

            if (inst->op == ir::opcode::add)
                return const_int{std::get<const_int>(*lhs).value + std::get<const_int>(*rhs).value};
            if (inst->op == ir::opcode::sub)
                return const_int{std::get<const_int>(*lhs).value - std::get<const_int>(*rhs).value};
            if (inst->op == ir::opcode::mul)
                return const_int{std::get<const_int>(*lhs).value * std::get<const_int>(*rhs).value};
            if (inst->op == ir::opcode::div) {
                // Hmm, should we crash the compiler if we encounter compile-time division by zero ?
                if (std::get<const_int>(*rhs).value == 0)
                    return std::nullopt;
                return const_int{std::get<const_int>(*lhs).value / std::get<const_int>(*rhs).value};
            }
            if (inst->op == ir::opcode::mod) {
                if (std::get<const_int>(*rhs).value == 0)
                    return std::nullopt;
                return const_int{std::get<const_int>(*lhs).value % std::get<const_int>(*rhs).value};
            }
            if (inst->op == ir::opcode::shl)
                return const_int{std::get<const_int>(*lhs).value << std::get<const_int>(*rhs).value};
            if (inst->op == ir::opcode::shr)
                return const_int{std::get<const_int>(*lhs).value >> std::get<const_int>(*rhs).value};

            assert(false && "should not reach here");
        }

        void run(ir::function &fn) override {
            std::cout << "constant folding" << '\n';
            for (auto &block: fn.blocks) {
                for (auto inst: block->instructions) {
                    if (!is_foldable(inst->op))
                        continue;

                    auto con = folded_value(inst);
                    if (!con)
                        continue;

                    // TODO: I guess this could be extracted to a separate function ?
                    for (auto operand : inst->operands)
                        erase_use(operand, inst);
                    inst->operands.clear();

                    if (auto ic = std::get_if<const_int>(&con.value())) {
                        inst->op = ir::opcode::iconst;
                        inst->data = ir::iconst_data{.value = ic->value};
                    }
                    if (auto bc = std::get_if<const_bool>(&con.value())) {
                        inst->op = ir::opcode::bconst;
                        inst->data = ir::bconst_data{.value = bc->value};
                    }
                }
            }
        }
    };
}
