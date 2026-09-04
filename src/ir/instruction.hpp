#pragma once
#include <vector>

#include "value.hpp"

/**
 * Linear code IR, meant to be transformed into a CFG
 */
namespace dungeon::ir {
    enum class opcode {
        iconst, bconst,

        add, sub, mul, div, mod, shl, shr, neg,

        eq, lt,

        lnot, land, lor,

        br, cond_br,

        alloca, load, store,

        call, ret,

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
            cond_br_data
        >;

        data_t data;

        bool has_side_effects() const;
    };
}
