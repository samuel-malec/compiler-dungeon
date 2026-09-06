#pragma once
#include "cfg2ssa.hpp"
#include "destroy_ssa.hpp"
#include "pass_manager.hpp"

namespace dungeon {
    inline analysis::pass_manager get_defult_pipeline() {
        analysis::pass_manager pm;
        // TODO: this design is kinda bad, because we are not checking the preconditions of individual passes anywhere
        pm.add(std::make_unique<cfg2ssa>());

        pm.add(std::make_unique<destroy_ssa>());
        return pm;
    }
}
