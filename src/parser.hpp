#pragma once

#include <format>
#include <iostream>
#include <vector>
#include <optional>
#include <memory>
#include <sstream>

#include "ast.hpp"
#include "lexer.hpp"

namespace dungeon
{

struct parser : token_sink
{
    using cat = token::cat_t;
    using type_k = ast::type_kind;
    using expr = ast::expr;
    using stmt = ast::stmt;
    using decl = ast::decl;
    using var_decl = ast::var_decl;
    using program = ast::program;

    token current;
    size_t pos = 0;
    source_ptr doc;
    lexer lex;

    parser( source_ptr doc ) : doc{ doc }, lex{ doc, *this } {}

    template <typename... Args >
    void error( Args... args )
    {
        std::stringstream buf{};
        ( ( ( buf << " " ) << args ), ... );
        throw std::runtime_error( buf.str() );
    }

    token require( cat c, std::string_view data = "" )
    {
        auto tok = fetch();
        if ( tok.cat != c )
            error( tok, "Expected category: ", c );
        
        if ( !data.empty() && tok.data != data )
            error( tok, "Expected: ", data );
        
        return tok;
    }

    type_k type_from_str( std::string_view data )
    {
        if ( data == "bool" )
            return ast::BOOL;
        if ( data == "int" )
            return ast::INT;
        if ( data == "unsigned" )
            return ast::UNSIGNED;
        if ( data == "void" )
            return ast::VOID;
        
        return ast::UNKNOWN;
    }

    bool match( cat c, std::string_view data = "" )
    {
        auto tok = peek();
        return tok.cat == c && tok.data == data;
    }
    
    token peek()
    {
        assert( !lex.empty() );
        while ( !lex.empty() && current.cat == cat::invalid )
            lex.next();

        return current;
    }

    token fetch()
    {
        auto rv = peek();
        current = {};
        return rv;
    }

    void push( token t ) override 
    { 
        current = std::move( t );
    }

    ast::program parse()
    {
        program prog{};
        while ( !lex.empty() )
        {
            std::optional< decl > d = parse_toplevel_decl();
            if ( !d.has_value() )
                error( "Unable to parse this" );

            prog.declarations.push_back( d.value() );
        }

        return prog;
    }

    // todo make the api cleaner and have only toplevel, stmt, expr, and internal functions in .cpp file
    std::optional< decl > parse_toplevel_decl();

    std::optional< decl > parse_fn_decl();
    
    std::vector< var_decl > parse_var_decl_list();

    std::optional< stmt > parse_stmt();

    std::optional< stmt > parse_block();
    
    std::optional< stmt > parse_loop_stmt();

    std::optional< stmt > parse_for();

    std::optional< stmt > parse_while();

    std::optional < stmt > parse_do_while();

    std::optional< stmt > parse_if();

    std::optional< stmt > parse_expr_stmt();
    
    std::optional< stmt > parse_ret();

    std::optional< stmt > parse_control_stmt();

    std::optional< stmt > parse_var_decl();
    
    std::optional< stmt > parse_assign();

    std::optional< expr > parse_expr();
};

}
