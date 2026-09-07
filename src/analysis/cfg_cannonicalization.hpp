#pragma once
#include "pass.hpp"

namespace dungeon {
    struct cfg_canonicalization : pass {
        void run(ir::function &fn) override {
            // TODO: remove unreachable code
            // TODO: collapse empty basic blocks and remove the labels;
        }
    };
}
