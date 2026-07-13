#include <iostream>
#include <cstring>
#include <stdexcept>

#include "common/file.hpp"
#include "common/pretty_printer.hpp"
#include "frontend/parser.hpp"
#include "frontend/semantic.hpp"
#include "frontend/token.hpp"

#include "middleend/hir/ast2hir.hpp"
#include "middleend/tac/hir2tac.hpp"

using config = std::pair< std::string, std::string >;

config parse_config( int argc, char* const* argv )
{
    if ( argc != 1 && argc != 3 )
        throw std::runtime_error( "Usage: ./compiler-dungeon file.c [-o output]\n" );
    
    std::string file_in = argv[ 0 ];
    std::string file_out = "a.s";
    if ( argc == 3 )
    {
        if ( strcmp( argv[ 1 ], "-o" ) )
            throw std::runtime_error( "invalid flag" );
        file_out = argv[ 2 ];
    }

    return { file_in, file_out };
}

int main( int argc, char* const* argv )
{
    using namespace dungeon;

    --argc;
    ++argv;
    try
    {
        const auto& [ in_name, out_name ] = parse_config( argc, argv );
        source_ptr doc = std::make_shared< source_file >( in_name, read_file( in_name ) ); 

        parser p{ doc };
        auto ast = p.parse();
        print::pretty_printer printer{};
        
        semantic_analyzer sa{};
        sa.run( ast );
        
        printer.print_ast( ast );

        hir::program hir = hir::lower_ast_to_hir( ast, sa.st );
        printer.print_hir( hir );

        tac::program tac_ir = tac::lower_to_tac( hir );
        printer.print_tac( tac_ir );
    }

    // todo: create something like diagnostic { warn, err } and catch this diagnostic& and print errors nicely
    catch( const std::exception& e )
    {
        std::cerr << "\033[1;31mexception:\033[m " << e.what() << "\n";
        return 1;
    }
}
