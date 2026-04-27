#pragma once

#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include "ast.hpp"

namespace dungeon
{

struct atom
{
    uint32_t idx;

    std::strong_ordering operator<=>( const atom& o ) const = default;
    bool operator==( const atom& o ) const = default;
};

struct symtab
{
    std::map< std::string, uint32_t, std::less<> > map;
    std::map< uint32_t, std::string > map_inverse;
    std::vector< std::set< uint32_t > > scopes;

    atom get( std::string_view name )
    {
        auto it = map.find( name );
        if ( it != map.end() )
            return atom{ .idx = it->second };

        uint32_t idx = static_cast< uint32_t >( map.size() );
        atom a{ .idx = idx };
        map[ std::string( name ) ] = idx;
        map_inverse[ idx ] = std::string( name );
        return a;
    }

    bool contains_name( std::string_view name ) { return map.contains( name ); }

    void create_scope() { scopes.push_back( {} ); }
    void close_scope() { scopes.pop_back() ; }
    void add_var( ast::var_decl& vd );
    bool find_var( std::string name );
};

struct semantic
{
    using expr = ast::expr;
    using stmt = ast::stmt;

    template < typename... Args >
    void error( Args... args )
    {
        std::stringstream buf{};
        ( ( ( buf << " " ) << args ), ... );
        throw std::runtime_error( buf.str() );
    }

    void resolve_expr( ast::expr& e, symtab& st )
    {
        switch ( e.cat )
        {
            case expr::num_lit:
            case expr::bool_lit:
            case expr::identifier:
            case expr::unary:
            case expr::binary:
            case expr::relational:
            case expr::assign:
            case expr::call:
            default:
                break;
        }
    }

    void resolve_stmt( ast::stmt& s, symtab& st )
    {
        switch ( s.cat )
        {
            case stmt::ret:
            case stmt::if_stmt:
            case stmt::for_stmt:
            case stmt::while_stmt:
            case stmt::do_while_stmt:
            case stmt::cont:
            case stmt::brk:
            case stmt::block:
            case stmt::var_dclr:
            case stmt::expr_stmt:
            default:
                break;
        }
    }

    void run( ast::program& prog )
    {
        symtab st{};
        for ( auto& d : prog.decls )
        {
            std::visit( [ this ]( auto&& arg )
            {
                using T = std::decay_t< decltype( arg ) >;
                // TODO: we should explicitly state that each function name must be unique in the whole program
                //       but I guess that it is implicated by the fact that we are compiling a subset of C
                //       but then again, if the language specification ever changes we need to come back to this place
                if constexpr( std::is_same_v< T, ast::fn_decl > )
                {
                    ast::fn_decl fn = arg;

                }
            }, d );
        }

    }
};

}
