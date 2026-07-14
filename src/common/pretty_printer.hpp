#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include "../frontend/ast.hpp"
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

    void print_expr( expr& e, int depth );

    void print_stmt( stmt& s, int depth );

    void print_ast( ast::program& ast );

    void print_hir_expr( const hir::expr& e, int depth );

    void print_hir_stmt( const hir::stmt& s, int depth );

    void print_hir( const hir::program& hir );

    std::string tac_arg_to_string( const tac::argument& arg );

    std::string tac_instr_symbolic( const tac::instr& i );

    void print_tac_inst( tac::instr& i )
    {
        std::holds_alternative< tac::label_data>( i.data ) ? pad( 2 ) : pad( 4 );
        std::cout << tac_instr_symbolic( i ) << '\n';
    }

    void print_tac( tac::program& tac );

    void export_to_dot( const cfg::cfg& graph, std::ostream& out );
};

}
