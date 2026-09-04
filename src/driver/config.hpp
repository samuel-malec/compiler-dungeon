#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>

namespace dungeon
{

enum class pipeline_stage {
    full,
    lexer,
    parser,
    semantic,
    typecheck,
    hir,
    ir,
    cfg,
    optimization,
};

inline pipeline_stage parse_stage(std::string_view stage_name)
{
    if (stage_name == "full" || stage_name == "all")
        return pipeline_stage::full;
    if (stage_name == "lexer" || stage_name == "lex")
        return pipeline_stage::lexer;
    if (stage_name == "parser" || stage_name == "parse")
        return pipeline_stage::parser;
    if (stage_name == "semantic" || stage_name == "sema")
        return pipeline_stage::semantic;
    if (stage_name == "typecheck" || stage_name == "type-check")
        return pipeline_stage::typecheck;
    if (stage_name == "hir")
        return pipeline_stage::hir;
    if (stage_name == "ir")
        return pipeline_stage::ir;
    throw std::runtime_error("unknown stage: " + std::string(stage_name));
}

struct config
{
    std::string in_name;
    std::string out_name;
    bool emit_tokens;
    bool emit_ast;
    bool emit_hir;
    bool emit_ir;
    bool emit_cfg;
    bool emit_son;
    pipeline_stage stage = pipeline_stage::full;
};

inline void help()
{
    std::cout << "Usage:\n"
              << "./compiler-dungeon file.ks [--stage lexer|parser|semantic|typecheck|hir|ir|full] [-o output]\n"
              << "--emit-tokens\n"
              << "--emit-ast\n"
              << "--emit-hir\n"
              << "--emit-tac\n"
              << "--emit-cfg\n"
              << "--emit-son\n";
}

inline config parse_config( int argc, char* const* argv )
{
    if ( argc < 1 )
        throw std::runtime_error( "Usage: ./compiler-dungeon file.ks [--stage ...]\n" );

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
    bool _emit_ir = false;
    bool _emit_cfg = false;
    bool _emit_son = false;
    auto stage = pipeline_stage::full;

    for ( int i = 1; i < argc; ++i )
    {
        if ( strcmp( argv[ i ], "-o" ) == 0 )
        {
            if ( i >= argc - 1 )
                throw std::runtime_error( "missing argument of -o" );

            ++i;
            file_out = argv[ i ];
        }
        else if ( strcmp( argv[ i ], "--stage" ) == 0 )
        {
            if ( i >= argc - 1 )
                throw std::runtime_error( "missing argument of --stage" );
            ++i;
            stage = parse_stage( argv[ i ] );
        }
        else if ( std::strncmp( argv[ i ], "--stage=", 8 ) == 0 )
        {
            stage = parse_stage( argv[ i ] + 8 );
        }
        else if ( strcmp( argv[ i ], "--emit-tokens" ) == 0 )
        {
            _emit_tokens = true;
        }
        else if ( strcmp( argv[ i ], "--emit-ast" ) == 0 )
        {
            _emit_ast = true;
        }
        else if ( strcmp( argv[ i ], "--emit-hir" ) == 0 )
        {
            _emit_hir = true;
        }
        else if ( strcmp( argv[ i ], "--emit-ir" ) == 0 )
        {
            _emit_ir = true;
        }
        else if ( strcmp( argv[ i ], "--emit-cfg" ) == 0 )
        {
            _emit_cfg = true;
        }
        else if ( strcmp( argv[ i ], "--emit-son" ) == 0 )
        {
            _emit_son = true;
        }
        else
        {
            throw std::runtime_error( std::string( "invalid flag: " ) + argv[ i ] );
        }
    }

    return {
            .in_name = file_in,
            .out_name = file_out,
            .emit_tokens = _emit_tokens,
            .emit_ast = _emit_ast,
            .emit_hir = _emit_hir,
            .emit_ir = _emit_ir,
            .emit_cfg = _emit_cfg,
            .emit_son = _emit_son,
            .stage = stage,
        };
}

}
