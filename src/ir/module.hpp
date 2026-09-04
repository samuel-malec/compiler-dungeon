#pragma once
#include <cstdint>
#include <vector>

#include "function.hpp"

namespace dungeon::ir {
    struct module {
        uint32_t main_idx;
        std::vector<function> funcs;
    };
}
