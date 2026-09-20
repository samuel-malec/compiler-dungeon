#pragma once
#include "dce.hpp"
#include "reg2mem.hpp"
#include "mem2reg.hpp"
#include "pass_manager.hpp"
#include "sccp.hpp"
#include "simplify_cfg.hpp"

namespace dungeon {
    // TODO: add a mechanism to check if preconditions of individual passes are satisfied
    inline analysis::pass_manager get_default_pipeline() {
        analysis::pass_manager pm;
        pass *cfg = pm.add(std::make_unique<simplify_cfg>());
        pm.add(std::make_unique<mem2reg>());
        pm.add(std::make_unique<sccp>());
        pm.add(std::make_unique<dce>());
        pm.repeat(cfg);
        pm.add(std::make_unique<reg2mem>());

        return pm;
    }
}
