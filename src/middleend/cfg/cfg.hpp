#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include <variant>

#include "../tac/tac.hpp"

namespace dungeon::cfg
{

using block_id = uint32_t;
using fn_id = uint32_t;
using ins_id = uint32_t;
using var_id = uint32_t;

struct basic_block;
using bb_ptr = std::unique_ptr< basic_block >;

struct phi_node
{
    var_id var;
    tac::tmp target;
    std::unordered_map< block_id, tac::tmp > incoming;
};

struct basic_block
{
    block_id id;
    std::vector< phi_node > phis;
    std::vector< tac::instr > instructions;
    std::vector< basic_block* > succ;
    std::vector< basic_block* > pred;
    std::vector< basic_block* > dom;
};

struct cfg
{
    bb_ptr entry;
    std::vector< bb_ptr > basic_blocks;
};

struct program
{
    std::map< fn_id, cfg > fns;
};

struct cfg_builder
{
    std::vector< bb_ptr > blocks;

    bool is_terminator( const tac::instr& i ) const
    {
        return std::holds_alternative< tac::ret_data >( i.data ) ||
               std::holds_alternative< tac::branch_data >( i.data ) ||
               std::holds_alternative< tac::jump_data >( i.data ); 
    }

    void connect( basic_block* from, basic_block* to )
    {
        from->succ.push_back( to );
        to->pred.push_back( from );
    }

    cfg build( const std::vector< tac::instr >& insns )
    {
        cfg res{};
        res.entry = std::make_unique< basic_block >();
        if ( insns.empty() )
            return res;

        // leaders
        std::vector< bool > is_leader( insns.size(), false );
        is_leader[ 0 ] = true;
        for ( int i = 0; i < insns.size(); ++i )
        {
            if ( is_terminator( insns[ i ] ) )
                if ( i < insns.size() - 1 )
                    is_leader[ i + 1 ] = true;
            
            if ( std::holds_alternative< tac::label_data >( insns[ i ].data ) )
                is_leader[ i ] = true;
        }

        std::vector< int > leader_indices;
        for ( int i = 0; i < insns.size(); ++i )
            if ( is_leader[ i ] )
                leader_indices.push_back( i );

        // I don't want to push labels as instructions cuz they pollute the cfg ...
        std::unordered_map< uint32_t, basic_block* > label_map;
        for ( int i = 0; i < leader_indices.size(); ++i ) 
        {
            int start = leader_indices[ i ];
            int end = i == leader_indices.size() - 1 ? insns.size() : leader_indices[ i + 1 ];
            bb_ptr curr = std::make_unique< basic_block >( );
            curr->id = res.basic_blocks.size();

            for ( int ptr = start; ptr < end; ++ptr )
            {
                if ( std::holds_alternative< tac::label_data >( insns[ ptr ].data ) )
                {
                    tac::label_data ld = std::get< tac::label_data >( insns[ ptr ].data );
                    label_map[ ld.id ] = curr.get();
                }
                else
                    curr->instructions.push_back( insns[ ptr ] );
            }

            res.basic_blocks.push_back( std::move( curr ) );
        }
        
        for ( int i = 0; i < res.basic_blocks.size(); ++i )
        {
            auto& bb = res.basic_blocks[ i ];
            if ( bb->instructions.empty() )
                continue;

            auto& terminator = bb->instructions.back();
            if ( std::holds_alternative< tac::jump_data >( terminator.data ) )
            {
                tac::jump_data jd = std::get< tac::jump_data >( terminator.data );
                connect( bb.get(), label_map.at( jd.label ) );
            }
            else if ( std::holds_alternative< tac::branch_data >( terminator.data ) )
            {
                tac::branch_data bd = std::get< tac::branch_data >( terminator.data );
                connect( bb.get(), label_map.at( bd.true_lab ) );
                connect( bb.get(), label_map.at( bd.false_lab ) );
            }
            else if ( std::holds_alternative< tac::ret_data >( terminator.data ) )
                continue;
            else if ( i + 1 < res.basic_blocks.size() )
                connect( bb.get(), res.basic_blocks[ i + 1 ].get() );
        }

        if ( !res.basic_blocks.empty() )
            connect( res.entry.get(), res.basic_blocks[ 0 ].get() );

        return res;
    }
};

inline program build_cfg( tac::program& tprog )
{
    program res{};

    for ( auto& fn : tprog.functions )
    {
        cfg_builder cb{};
        res.fns[ fn.name ] = cb.build( fn.body );
    }
    
    return res;
}

};
