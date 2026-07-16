#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include "../frontend/ast.hpp"
#include "../frontend/semantic.hpp"
#include "../middleend/cfg/cfg.hpp"
#include "../middleend/tac/tac.hpp"
#include "../middleend/hir/hir.hpp"

// Right now this is a utility printer for debugging purposes, later
// we should add a more sophisticated logging system, which would allow us to
// log information about compiler phases, such as duration of the phase,
// and specific metadata.

namespace dungeon::print
{

struct pretty_printer
{
    using expr = ast::expr;
    using stmt = ast::stmt;
    using toplevel = ast::toplevel;
    using var_decl = ast::var_decl;
    using fn_decl = ast::fn_decl;
    using enum_decl = ast::enum_decl;
    using struct_decl = ast::struct_decl;
    using atom_map = std::map< uint32_t, std::string >;

    std::string indent( int depth )
    {
        return std::string( depth * 2, ' ' );
    }

    void pad( int depth )
    {
        std::cout << indent( depth );
    }

    std::string tac_tmp_to_string( const tac::tmp& t )
    {
        return "t" + std::to_string( t.id );
    }

    std::string tac_loc_to_string( const tac::loc& t, const atom_map& am )
    {
        if ( std::holds_alternative< tac::tmp >( t ) )
            return tac_tmp_to_string( std::get< tac::tmp >( t ) );
        
        tac::var v = std::get< tac::var >( t );
        return am.at( v.source_name_idx ) + std::to_string( v.id );
    }

    void print_expr( expr& e, int depth );

    void print_stmt( stmt& s, int depth );

    void print_ast( ast::program& ast );

    void print_hir_expr( hir::expr& e, int depth, const atom_map& am );

    void print_hir_stmt( hir::stmt& s, int depth, const atom_map& am );

    void print_hir( hir::program& hir, const atom_map& am );
    
    std::string tac_arg_to_string( tac::argument& arg, const atom_map& am );

    std::string tac_instr_symbolic( tac::instr& i, const atom_map& am );

    void print_tac_inst( tac::instr& i, const atom_map& am )
    {
        std::holds_alternative< tac::label_data>( i.data ) ? pad( 2 ) : pad( 4 );
        std::cout << tac_instr_symbolic( i, am ) << '\n';
    }

    void print_tac( tac::program& tac, const atom_map& am );

    void export_to_dot( cfg::cfg& graph, std::ostream& out, const atom_map& am );
};

}
