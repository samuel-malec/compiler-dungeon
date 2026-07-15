#pragma once

#include "cfg.hpp"

namespace dungeon::cfg
{

struct ssa_builder
{

    // TODO: compute the dominance trees of a cfg
    void dom_tree( cfg& graph )
    {
        graph.entry->dom.push_back( graph.entry.get( ) );

    }

    // todo: compute dominance frontiers
    void dom_frontiers( cfg& graph );


    void insert_phi( cfg& graph );

    void rename( cfg& graph );

    cfg transform_ssa( cfg& graph );
};

}
