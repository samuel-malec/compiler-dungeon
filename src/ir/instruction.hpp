#pragma once
#include <variant>
#include <vector>

#include "basic_block.hpp"
#include "../sema/symbol_table.hpp"
#include "value.hpp"

namespace dungeon {
    struct basic_block;
    struct block_id;
}

namespace dungeon::ir {
    enum class opcode {
        iconst, bconst,

        add, sub, mul, div, mod, shl, shr, neg,

        eq, lt,

        lnot,

        br, cond_br,

        alloca, load, store,

        call, ret, param,

        phi,

        label,
    };

    struct iconst_data {
        uint64_t value;
    };

    struct bconst_data {
        bool value;
    };

    struct call_data {
        sema::fn_id target;
    };

    struct label_data {
        uint32_t id;
    };

    // TODO: Investigate whetehr these branch_ids in br_data and cond_br_data are consistent with the basic block ids
    struct br_data {
        uint32_t branch_id;
    };

    struct cond_br_data {
        uint32_t true_branch;
        uint32_t false_branch;
    };

    struct param_data {
        uint32_t index;
    };

    struct phi_data {
        std::map<basic_block *, value *> incoming;
    };

    struct instruction {
        opcode op;
        value *result = nullptr;
        std::vector<value *> operands;
        basic_block *parent = nullptr;

        using data_t = std::variant<
            std::monostate,
            iconst_data,
            bconst_data,
            call_data,
            label_data,
            br_data,
            cond_br_data,
            param_data,
            phi_data
        >;

        data_t data;

        bool has_side_effects() const {
            switch (op) {
                case opcode::alloca:
                case opcode::store:
                case opcode::call:
                case opcode::br:
                case opcode::cond_br:
                case opcode::ret:
                    return true;
                default:
                    return false;
            }
        }

        bool is_terminator() const {
            return op == opcode::br || op == opcode::cond_br || op == opcode::ret;
        }

        void erase_use(value *operand) const {
            if (!operand)
                return;
            auto &users = operand->users;
            std::erase(users, this);
        }

        void erase_operands() {
            for (auto operand: operands)
                erase_use(operand);
            operands.clear();
        }
    };

    inline void replace_all_uses_with(value *old_val, value *new_val) {
        for (instruction *user: old_val->users) {
            for (auto &op: user->operands)
                if (op == old_val)
                    op = new_val;
            new_val->users.push_back(user);
        }
        old_val->users.clear();
    }
}
