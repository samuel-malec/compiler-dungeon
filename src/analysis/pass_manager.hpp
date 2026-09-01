#pragma once
#include <memory>
#include <vector>

namespace dungeon::analysis {
    struct pass {
        virtual ~pass() = default;

        virtual std::string_view name() = 0;

        virtual void run() = 0;
    };

    struct pass_manager {
        void add_pass(std::unique_ptr<pass> pass) { passes.push_back(std::move(pass)); }
        std::vector<std::unique_ptr<pass> > passes;
    };
}
