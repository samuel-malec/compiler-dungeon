#pragma once
#include "constant_folding.hpp"
#include "dce.hpp"
#include "destroy_ssa.hpp"
#include "mem2reg.hpp"
#include "pass_manager.hpp"

namespace dungeon {
    inline analysis::pass_manager get_default_pipeline() {
        analysis::pass_manager pm;
        // TODO: add a mechanism to check if preconditions of individual passes are satisfied

        pm.add(std::make_unique<mem2reg>());
        pm.add(std::make_unique<dce>());
        pm.add(std::make_unique<constant_folding>());
        pm.add(std::make_unique<destroy_ssa>());
        
        return pm;
    }
}
