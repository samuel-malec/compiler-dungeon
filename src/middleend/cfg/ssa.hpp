#pragma once

#include "cfg.hpp"
#include "../tac/tac.hpp"

namespace dungeon::cfg
{

struct ssa_builder
{
    using block_id = uint32_t;
    using order = std::vector< basic_block* >;

    void transform_ssa( cfg& graph );
};

}
