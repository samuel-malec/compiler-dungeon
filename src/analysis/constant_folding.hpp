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
                   op == ir::opcode::lnot || op == ir::opcode::lt ||
                   op == ir::opcode::eq;
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
            if (inst->op == ir::opcode::eq) {
                auto blhs = std::get_if<const_bool>(&*lhs);
                auto brhs = std::get_if<const_bool>(&*lhs);
                if ((blhs && !brhs) || (!blhs && brhs)) {
                    // this should never happen in our type system, we will think if we want to change this later
                    assert(false && "should not reach here");
                }
                if (blhs && brhs)
                    return const_bool{.value = blhs->value == brhs->value};

                auto ilhs = std::get_if<const_int>(&*lhs);
                auto irhs = std::get_if<const_int>(&*lhs);
                if ((ilhs && !irhs) || (!ilhs && irhs)) {
                    // this should never happen in our type system, we will think if we want to change this later
                    assert(false && "should not reach here");
                }

                assert(ilhs && irhs);
                return const_bool{.value = ilhs->value == irhs->value};
            }
            if (inst->op == ir::opcode::lt) {
                auto ilhs = std::get_if<const_int>(&*lhs);
                auto irhs = std::get_if<const_int>(&*rhs);
                assert(ilhs && irhs);
                return const_bool{.value = ilhs->value < irhs->value};
            }

            assert(false && "should not reach here");
        }

        void run(ir::function &fn) override {
            for (auto &block: fn.blocks) {
                for (auto inst: block->instructions) {
                    if (!is_foldable(inst->op))
                        continue;

                    auto con = folded_value(inst);
                    if (!con)
                        continue;

                    ir::erase_operands(inst);
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

        std::string description() override {
            return "Perform compile-time evaluation of constant computation and replace variables with their compile-time constant values";
        }
    };
}
