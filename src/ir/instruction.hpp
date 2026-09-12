#pragma once
#include <variant>
#include <vector>

#include "../sema/symbol_table.hpp"
#include "value.hpp"

namespace dungeon {
    struct block_id;
}

// TODO: make phi an actual instruction
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
        std::map<block_id, value *> incoming;
    };

    struct instruction {
        opcode op;

        value *result = nullptr;

        std::vector<value *> operands;

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
    };

    inline bool is_terminator(const opcode op) {
        return op == opcode::br || op == opcode::cond_br || op == opcode::ret;
    }

    inline bool is_terminator(const instruction *i) {
        return is_terminator(i->op);
    }

    inline void replace_all_uses_with(value *old_val, value *new_val) {
        for (instruction *user: old_val->users) {
            for (auto &op: user->operands)
                if (op == old_val)
                    op = new_val;
            new_val->users.push_back(user);
        }
        old_val->users.clear();
    }

    static void erase_use(value *v, const instruction *user) {
        if (!v)
            return;
        auto &users = v->users;
        std::erase(users, user);
    }
}
