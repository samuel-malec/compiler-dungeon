#pragma once
#include <memory>

#include "instruction.hpp"

namespace dungeon::ir {
    using ins_ptr = std::unique_ptr<instruction>;
    using value_ptr = std::unique_ptr<value>;

    struct function {
        std::vector<const type *> param_types;
        const type *return_type;

        std::vector<ins_ptr> instructions;
        std::vector<value_ptr> values;
    };
}
