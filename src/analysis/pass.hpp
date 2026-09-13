#pragma once
#include "../ir/function.hpp"

namespace dungeon {
    struct pass {
        virtual void run(ir::function &fn) = 0;

        virtual ~pass() = default;
    };
}
