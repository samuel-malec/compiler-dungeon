#pragma once

namespace dungeon {
    struct analysis {
        virtual void before(cfg &graph) = 0;

        virtual void run(cfg &graph) = 0;

        virtual void after(cfg &graph) = 0;

        void ~analysis() = default;
    };
}

}
