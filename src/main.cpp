#include <iostream>

#include "driver/compiler.hpp"
#include "driver/config.hpp"

int main( int argc, char* const* argv )
{
    --argc;
    ++argv;
    try
    {
        auto conf = dungeon::parse_config( argc, argv );
        dungeon::compiler compiler{};
        compiler.run( conf );
    }

    catch( const std::exception& e )
    {
        std::cerr << "\033[1;31mexception:\033[m " << e.what() << "\n";
        return 1;
    }
}
