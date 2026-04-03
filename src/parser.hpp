#pragma once

#include <vector>

#include "lexer.hpp"

namespace dungeon
{

struct parser : token_sink
{
    std::vector< token > tokens;
    source_ptr doc;
    lexer l;

    parser( source_ptr doc ) : doc{ doc }, l{ doc, *this } {}

    void push( token t ) override { tokens.push_back( std::move( t ) ); }

    void parse();
};

}
