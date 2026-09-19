#pragma once

namespace dungeon::lattice {
    template<typename T>
    struct lattice {
        virtual T get_top() = 0;

        virtual T get_bot() = 0;

        virtual T join(T lhs, T rhs) = 0;

        virtual T meet(T lhs, T rhs) = 0;

        virtual ~lattice() = default;
    };
}
