#pragma once
#include <iostream>

#include "pass.hpp"

namespace dungeon {
    struct constant_folding : pass {
        void run(ir::function &fn) override {
            std::cout << "constant folding" << '\n';
        }
    };
}
