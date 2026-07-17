#pragma once

#include "cfg.hpp"

namespace dungeon::cfg
{

struct ssa_builder
{
    using block_id = uint32_t;
    using order = std::vector< basic_block* >;

    order reverse_postorder( cfg& graph );
    
    void compute_dom_tree( cfg& graph );

    void compute_dom_frontiers( cfg& graph );

    void insert_phi( cfg& graph );

    void rename( cfg& graph );

    void transform_ssa( cfg& graph );

};

}
