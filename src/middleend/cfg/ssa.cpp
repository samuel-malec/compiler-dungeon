#include <algorithm>
#include <cassert>
#include <iostream>

#include "ssa.hpp"

namespace dungeon::cfg
{
    using order = std::vector< basic_block* >;

    void dfs( basic_block* bb, order& res, std::set< block_id >& visited )
    {
        visited.insert( bb->id );
        for ( basic_block* ptr : bb->succ )
            if ( !visited.contains( ptr->id ) )
                dfs( ptr, res, visited );
        
        res.push_back( bb );
    }

    order ssa_builder::reverse_postorder( cfg& graph )
    {
        order res{};
        std::set< block_id > visited{};
        dfs( graph.entry, res, visited );
        std::reverse( res.begin(), res.end() );

        for ( int i = 0; i < res.size(); ++i )
            res[ i ]->postorder_id = i;
        
        return res;
    }
    
    basic_block* intersect( cfg& graph, basic_block* bb1, basic_block* bb2 )
    {
        while ( bb1->postorder_id != bb2->postorder_id )
        {
            while ( bb1->postorder_id > bb2->postorder_id )
                bb1 = bb1->idom;
            while ( bb2->postorder_id > bb1->postorder_id )
                bb2 = bb2->idom;
        }

        return bb1;
    }

    void ssa_builder::compute_dom_tree( cfg& graph )
    {
        order rpo = reverse_postorder( graph );
        
        // computing idoms
        for ( auto& bb : graph.basic_blocks )
            bb->idom = nullptr;
        graph.entry->idom = graph.entry;
        bool changed = true;

        while ( changed )
        {
            changed = false;
            for ( int i = 0; i < rpo.size(); ++i )
            {
                if ( rpo[ i ]->id == graph.entry->id )
                    continue;

                basic_block* b = rpo[ i ];
                basic_block* new_idom = nullptr;

                for ( basic_block* pred : b->pred )
                {
                    if ( pred->idom != nullptr )
                    {
                        new_idom = pred;
                        break;
                    }
                }

                for ( basic_block* p : b->pred )
                {
                    if ( p->id == new_idom->id )
                        continue;
                    
                    if ( p->idom != nullptr )
                        new_idom = intersect( graph, p, new_idom );
                }

                if ( b->idom != new_idom )
                {
                    b->idom = new_idom;
                    changed = true;
                }
            }
        }

        std::cout << "printing idom\n";
        for ( auto& bb : graph.basic_blocks )
            std::cout << "idom( bb" << bb->id << " ) = bb" << bb->idom->id << '\n';
    }

    void ssa_builder::compute_dom_frontiers( cfg& graph )
    {
        for ( auto& bb : graph.basic_blocks )
        {
            if ( bb->pred.size() < 2 )
                continue;
            
            for ( auto& p : bb->pred )
            {
                basic_block* runner = p;
                while ( runner != bb->idom )
                {
                    runner->df.push_back( bb.get() );
                    runner = runner->idom;
                }
            }
        }

        for ( auto& bb : graph.basic_blocks )
        {
            std::cout << "df( bb " << bb->id << " ) = { ";
            for ( auto& t : bb->df )
                std::cout << t->id << " ";
            std::cout << "}\n";
        }
    }

    void ssa_builder::insert_phi( cfg& graph )
    {
        std::set< tac::value > vars;
        std::map< uint32_t, std::set< basic_block* > > def_blocks;
        for ( auto& bb : graph.basic_blocks )
        {
            for ( auto& ins : bb->instructions )
            {
                std::optional< tac::value > val = ins.get_target();
                if ( !val )
                    continue;
                
                tac::value v = val.value();
                vars.insert( v );
                def_blocks[ v.id ].insert( bb.get() );
            }
        }

        for ( auto& [ vid, defs ] : def_blocks )
        {
            std::set< basic_block* > has_phi;
            std::set< basic_block* > on_worklist( defs.begin(), defs.end() );
            std::vector< basic_block* > worklist( defs.begin(), defs.end() );

            while ( !worklist.empty() )
            {
                basic_block* n = worklist.back();
                worklist.pop_back();

                for ( basic_block* y : n->df )
                {
                    if ( has_phi.contains( y ) )
                        continue;
                    
                    y->phis.push_back( phi_node{ .base_id = vid } );
                    has_phi.insert( y );

                    if ( !on_worklist.contains( y ) )
                    {
                        on_worklist.insert( y );
                        worklist.push_back( y );
                    }
                }
            }
        }

    }

    void ssa_builder::rename( cfg& graph )
    {

    }

    void ssa_builder::transform_ssa( cfg& graph )
    {
        compute_dom_tree( graph );
        compute_dom_frontiers( graph );
        insert_phi( graph );
        rename( graph );
    }
}
