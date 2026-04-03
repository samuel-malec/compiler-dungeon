#include <iostream>

#include "parser.hpp"

namespace dungeon
{
    void parser::parse()
    {
        l.lex();
        for ( const auto& t : tokens )
            std::cout << t << '\n';
    }
}
