#pragma once

#include "cfg.hpp"

namespace dungeon::cfg
{

enum color
{ 
    white, 
    gray,
    black
};

struct ssa_builder
{
    using block_id = uint32_t;
    using order = std::vector< block_id >;

    // TODO: compute reverse-postorder
    order rpo( cfg& graph );
    
    // TODO: compute the dominance trees of a cfg
    void dom_tree( cfg& graph )
    {

    }

    // todo: compute dominance frontiers
    void dom_frontiers( cfg& graph );


    void insert_phi( cfg& graph );

    void rename( cfg& graph );

    void transform_ssa( cfg& graph ) 
    {
    }

};

}
