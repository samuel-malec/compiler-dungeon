#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>

namespace dungeon
{

struct config
{
    std::string in_name;
    std::string out_name;
    bool emit_tokens;
    bool emit_ast;
    bool emit_hir;
    bool emit_tac;
    bool emit_cfg;
    bool emit_son;
};

inline void help()
{
    std::cout << "Usage:\n"
              << "./compiler-dungeon file.ks [-o output]\n"
              << "--emit-tokens\n"
              << "--emit-ast\n"
              << "--emit-hir\n"
              << "--emit-tac\n"
              << "--emit-cfg\n"
              << "--emit-son\n";
}

inline config parse_config( int argc, char* const* argv )
{
    if ( argc < 1 || argc > 7 )
        throw std::runtime_error( "Usage: ./compiler-dungeon file.ks [-o output]\n" );
    
    if ( strcmp( argv[ 0 ], "-h" ) == 0 )
    {
        help();
        exit( 0 );
    }

    std::string file_in = argv[ 0 ];
    std::string file_out = "a.s";
    bool _emit_tokens = false;
    bool _emit_ast = false;
    bool _emit_hir = false;
    bool _emit_tac = false;
    bool _emit_cfg = false;
    bool _emit_son = false;
    
    for ( int i = 1; i < argc; ++i )
    {
        if ( strcmp( argv[ i ], "-o" ) == 0 )
        {
            if ( i >= argc - 1 )
                throw std::runtime_error( "missing argument of -o" );

            ++i;
            file_out = argv[ i ];
        }
        else if ( strcmp( argv[ i ], "--emit-tokens" ) == 0 )
        {
            _emit_tokens = true;
            continue;
        }
        else if ( strcmp( argv[ i ], "--emit-ast" ) == 0 )
        {
            _emit_ast = true;
            continue;
        }
        else if ( strcmp( argv[ i ], "--emit-hir" ) == 0 )
        {
            _emit_hir = true;
            continue;
        }
        else if ( strcmp( argv[ i ], "--emit-tac" ) == 0 )
        {
            _emit_tac = true;
            continue;
        }
        else if ( strcmp( argv[ i ], "--emit-cfg" ) == 0 )
        {
            _emit_cfg = true;
            continue;
        }
        else if ( strcmp( argv[ i ], "--emit-son" ) == 0 )
        {
            _emit_son = true;
            continue;
        }

        throw std::runtime_error( "invalid flag" );
    }

    return { 
            .in_name = file_in,
            .out_name = file_out,
            .emit_tokens = _emit_tokens,
            .emit_ast = _emit_ast,
            .emit_hir = _emit_hir,
            .emit_tac = _emit_tac,
            .emit_cfg = _emit_cfg,
            .emit_son = _emit_son
        };
}

}
