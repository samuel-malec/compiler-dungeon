#pragma once

#include <string>
#include <memory>

namespace dungeon
{
    struct source_file
    {
        std::string name, data;
            
        source_file( std::string name, std::string data ) :
                name{ std::move( name ) },
                data{ std::move( data ) }
        {}
    };

    using source_ptr = std::shared_ptr< source_file >;

    struct location
    {
        source_ptr doc;
        int line = 0, col = 0, byte = 0;
    };
}
