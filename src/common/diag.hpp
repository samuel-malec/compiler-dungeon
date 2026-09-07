#pragma once
#include <sstream>

namespace dungeon::diag
{
    // NOTE TO SELF: some useful colors
    // std::cerr << "\033[1;32mSuccess!\033[m\n";
    // std::cerr << "\033[1;31mError!\033[m\n";
    // std::cerr << "\033[1;33mWarning!\033[m\n";
    template < typename... Args >
    void error( Args... args )
    {
        std::stringstream buf{};
        ( ( ( buf << " " ) << args ), ... );
        throw std::runtime_error( buf.str() );
    }

struct diag
{

};
}
