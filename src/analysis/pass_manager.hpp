#pragma once
#include <memory>
#include <vector>

#include "pass.hpp"
#include "../ir/module.hpp"

namespace dungeon::analysis {
    struct pass_manager {
        std::vector<std::unique_ptr<pass> > owned;
        std::vector<pass *> schedule;

        pass *add(std::unique_ptr<pass> pass) {
            owned.push_back(std::move(pass));
            auto raw = owned.back().get();
            schedule.push_back(raw);
            return raw;
        }

        void repeat(pass *p) {
            schedule.push_back(p);
        }

        void run(ir::function &fn) {
            for (auto pass: schedule)
                pass->run(fn);
        }
    };
}
