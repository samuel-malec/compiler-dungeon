#pragma once
#include "../ir/function.hpp"

namespace dungeon {
    struct pass {
        // TODO: think whether this is a good design choice; for example graal has before() run() after() for setup and cleanup of analysis
        virtual void run(ir::function &fn) = 0;

        virtual ~pass() = default;
    };
}
