#pragma once
#include <vector>

#include "basic_block.hpp"
#include "value.hpp"

/**
 * Linear code IR, meant to be transformed into a CFG
 */
namespace dungeon::ir {
    enum opcode {
        iconst, bconst,

        add, sub, mul, div, neg,

        eq, neq, leq, lt, geq, gt,

        lnot, land, lor,

        br, cond_br,

        load, store,

        call, ret,

        phi,
    };

    struct iconst_data {
        uint64_t value;
    };

    struct bconst_data {
        bool value;
    };

    struct instruction {
        opcode op;

        value *result = nullptr;

        std::vector<value *> operands;

        basic_block *parent = nullptr;

        // TODO: complete these data structures
        using data_t = std::variant<
            iconst_data,
            bconst_data>;

        data_t data;

        bool has_side_effects() const;
    };

    struct function {
    };
}
