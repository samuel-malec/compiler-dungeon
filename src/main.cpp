#include <iostream>
#include <cstring>
#include <stdexcept>


int main( int argc, char* const* argv )
{
    --argc;
    ++argv;

    if ( argc != 1 && argc != 3 )
    {
        std::cout << "Usage: ./vcc file.vj [-o output]\n";
        return 1;
    }


    try
    {
        std::string file_in = argv[ 1 ];
        std::string file_out = "a.s";
        if ( argc == 3 )
        {
            if ( strcmp( argv[ 1 ], "-o" ) )
                throw std::runtime_error( "invalid flag" );
            file_out = argv[ 2 ];
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    
    std::cout << "hello world\n";
}
