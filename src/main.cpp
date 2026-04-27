#include <iostream>
#include <cstring>
#include <stdexcept>

#include "common/file.hpp"
#include "token.hpp"
#include "parser.hpp"
#include "pretty_printer.hpp"
#include "semantic.hpp"

using config = std::pair< std::string, std::string >;

config parse_config( int argc, char* const* argv )
{
    if ( argc != 1 && argc != 3 )
        throw std::runtime_error( "Usage: ./vcc file.vj [-o output]\n" );
    
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
        const auto& [ in_name, out_name ]= parse_config( argc, argv );
        std::string data = read_file( in_name );
        source_ptr doc = std::make_shared< source_file >( in_name, read_file( in_name ) ); 
        parser p{ doc };
        auto prog = p.parse();
        print::pretty_printer printer{};
        printer.print_ast( prog );
        semantic sem{};
        sem.run( prog );
    }
    catch( const std::exception& e )
    {
        std::cerr << "\033[1;31mexception:\033[m " << e.what() << "\n";
        return 1;
    }
}
