#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <string>

inline std::string read_file( std::istream& in )
{
    std::string out;
    int pos = 0;

    while ( in )
    {
        out.resize( pos + 1024 );
        in.read( out.data() + pos, 1024 );
        out.resize( pos += in.gcount() );
    }

    return out;
}

inline std::string read_file( std::string path )
{
    std::ifstream file( path, std::ios::in );
    if ( !file.is_open() )
        throw std::runtime_error( "couldn't open file at: " + path );

    std::stringstream sstream;
    sstream << file.rdbuf();
    return sstream.str();
}
