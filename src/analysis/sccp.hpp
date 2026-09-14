#pragma once
#include "pass.hpp"

namespace dungeon {
    struct sccp : pass {
        void run(ir::function &fn) override {
        }

        std::string description() override {
            return "Sparse Conditional Constant Propagation";
        }
    };
}
