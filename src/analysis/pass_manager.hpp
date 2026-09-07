#pragma once
#include <memory>
#include <vector>

#include "pass.hpp"
#include "../ir/module.hpp"

namespace dungeon::analysis {
    struct pass_manager {
        std::vector<std::unique_ptr<pass> > passes;

        void add(std::unique_ptr<pass> pass) { passes.push_back(std::move(pass)); }

        void run(ir::function &fn) {
            for (auto& pass: passes)
                    pass->run(fn);
        }
    };
}
