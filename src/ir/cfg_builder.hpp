#pragma once
#include <functional>

#include "cfg.hpp"
#include "function.hpp"

namespace dungeon::ir {
    struct cfg_builder {
        function fn;
        cfg graph{};

        void connect(basic_block *from, basic_block *to) {
            from->succ.push_back(to);
            to->pred.push_back(from);
        }
    };
}
