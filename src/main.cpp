#include <iostream>

#include "driver/compiler.hpp"
#include "driver/config.hpp"

int main( int argc, char* const* argv )
{
    --argc;
    ++argv;
    dungeon::compiler compiler{};
    try
    {
        auto conf = dungeon::parse_config( argc, argv );
        compiler.run( conf );
    }
    // todo: create something like diagnostic { warn, err } and catch this diagnostic& and print errors nicely
    catch( const std::exception& e )
    {
        std::cerr << "\033[1;31mexception:\033[m " << e.what() << "\n";
        return 1;
    }
}
