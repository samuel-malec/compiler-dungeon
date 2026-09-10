#pragma once
#include "pass.hpp"

namespace dungeon {
    struct dce : pass {
        void run(ir::function &fn) override {
            std::cout << "running dead code elimination\n";
        }
    };
}
