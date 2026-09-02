#pragma once
#include <cstdint>

#include "instruction.hpp"
#include "../sema/types.hpp"

namespace dungeon::ir {

    struct value {
        uint32_t id;
        const type* type;

        std::vector< instruction* > usages;
    };
}
