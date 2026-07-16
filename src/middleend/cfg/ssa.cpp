#include <cassert>

#include "ssa.hpp"

namespace dungeon::cfg
{
    void dfs( basic_block* bb, ssa_builder::order& res, std::map< block_id, color >& colors )
    {
        // colors[ bb->id ] = gray;

        // color[ bb->id ] = black;
        // res.push_back( bb->id );
    }

    std::vector< block_id > ssa_builder::rpo( cfg& graph )
    {
        assert( false && "unimplemented" );
    }
}
