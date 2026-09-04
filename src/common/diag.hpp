#pragma once
#include <sstream>

namespace dungeon::diag
{
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
