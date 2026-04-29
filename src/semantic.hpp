#pragma once

#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string>
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

template < typename... Args >
void error( Args... args )
{
    std::stringstream buf{};
    ( ( ( buf << " " ) << args ), ... );
    throw std::runtime_error( buf.str() );
}

// todo: this deserves a much better implementation, move type specic stuff into types.hpp
//       where the type hierarchy and management shall be implemented in greater detail in future

struct fn_signature
{
    ast::type_kind ret_type;
    std::vector< ast::type_kind > param_types;
};

using type = std::variant< fn_signature, ast::type_kind >;

struct symtab
{
    struct scope_kind
    {
        enum cat_t 
        {
            function,
            stmt,
            global,
        } cat;
        
        ast::type_kind fn_ret_type;
        std::string fn_name;
        ast::stmt::cat_t scat;
    };

    std::map< std::string, uint32_t, std::less<> > map;
    std::vector< std::map< uint32_t, type > > scopes;
    std::vector< scope_kind > kinds;

    atom get( std::string_view name )
    {
        auto it = map.find( name );
        if ( it != map.end() )
            return atom{ .idx = it->second };

        uint32_t idx = static_cast< uint32_t >( map.size() );
        atom a{ .idx = idx };
        map[ std::string( name ) ] = idx;
        return a;
    }

    bool contains_name( std::string_view name ) { return map.contains( name ); }

    void create_scope() 
    {
        // todo: if this is the scope enclosing the function body, we should pass parameters to this scope also 
        scopes.push_back( {} );
    }

    void close_scope() { scopes.pop_back(); }

    void push_kind( scope_kind sk ) { kinds.push_back( sk ); }

    scope_kind pop_kind()
    {
        scope_kind sk = kinds.back();
        kinds.pop_back();
        return sk;
    }
    
    void add_var( std::string_view name, ast::type_kind tk )
    {
        atom a = get( name );
        if ( scopes.back().contains( a.idx ) )
            error( "re-definition of identifier: ", name );

        map[ std::string( name ) ] = a.idx;
        scopes.back().emplace( a.idx, tk );
    }

    std::optional< type > find_var( std::string name )
    {
        atom a = get( name );
        for ( int i = scopes.size() - 1; i >= 0; -- i )
        {
            auto it = scopes[ i ].find( a.idx );
            if ( it == scopes[ i ].end() )
                continue;
            
            return it->second;
        }

        return {};
    }
    
    bool within_loop() const
    {
        for ( int i = kinds.size() - 1; i >= 0; -- i )
        {
            scope_kind sk = kinds[ i ];
            if ( sk.cat != scope_kind::stmt )
                continue;
            if ( sk.scat == ast::stmt::for_stmt || sk.scat == ast::stmt::while_stmt || 
                 sk.scat == ast::stmt::do_while_stmt )
                return true;
        }
        return false;
    }

    std::optional< ast::type_kind > enclosing_fn_ret_type() const
    {
        for ( int i = kinds.size() - 1; i >= 0; -- i  )
        {
            scope_kind sk = kinds[ i ];
            if ( sk.cat == scope_kind::function )
                return sk.fn_ret_type;
        }

        return {};
    }
};

struct semantic
{
    using expr = ast::expr;
    using stmt = ast::stmt;
    using type_kind = ast::type_kind;
    using scope_kind = symtab::scope_kind;

    bool resolve_op( type lhs, type rhs, ast::op_kind op ) { return true; }
    
    bool compatible( type to, type from ) { return true; }

    type resolve_expr( ast::expr& e, symtab& st )
    {
        switch ( e.cat )
        {
            // TODO: add a way to create unsigned literals in the language 
            case expr::num_lit:
            {
                return type_kind::INT;
            }

            case expr::bool_lit:
            {
                return type_kind::BOOL;
            }

            case expr::identifier:
            {
                auto v = st.find_var( std::string( e.id ) );
                if ( !v )
                    error( "variable used before definition", e.id );
                return v.value();             
            }

            case expr::unary:
            {
                resolve_expr( e[ 0 ], st );
                break;
            }

            case expr::binary:
            case expr::relational:
            {
                auto lhs = resolve_expr( e[ 0 ], st );
                auto rhs = resolve_expr( e[ 1 ], st );
                if ( !resolve_op( lhs, rhs, e.op ) )
                    error( "invalid types in operands in operation", e.op );
                return type_kind::VOID;
            }

            case expr::assign:
            {
                auto lhs = resolve_expr( e[ 0 ], st ); 
                auto rhs = resolve_expr( e[ 1 ], st );
                if ( !compatible( lhs, rhs ) )
                    error( "invalid types in assignments" );
                return rhs;
            }

            case expr::call:
            {
                std::string id = std::string( e.subs[ 0 ].id );
                auto callee = st.find_var( id );
                if ( !callee )
                    error( "unknown identifier: ", id );
                
                if ( !std::holds_alternative< fn_signature >( callee.value() ) )
                    error( "expected a function call but insead got a primitive type" );
                
                fn_signature fsig = std::get< fn_signature >( callee.value() );
                int actuals_amount = e.subs.size() - 1;
                if ( actuals_amount != fsig.param_types.size() )
                    error( id, " expected ", fsig.param_types.size(), " arguments, but got ", actuals_amount, " instead" );

                for ( int i = 1; i < e.subs.size(); ++ i )
                    if ( !compatible( fsig.param_types[ i - 1 ], resolve_expr( e.subs[ i ], st ) ) )
                        error( "invalid types in function call" );

                return e.subs[ 0 ].type;
            }

            default:
                error( "should not reach here" );
                break;
        }
        
        return ast::type_kind::VOID;
    }

    void resolve_stmt( ast::stmt& s, symtab& st )
    {
        st.push_kind( { .cat = scope_kind::stmt, .scat = s.cat } );
        switch ( s.cat )
        {
            // TODO: check that we are actually returning something
            case stmt::ret:
            {
                auto fn_ret_type = st.enclosing_fn_ret_type();
                if ( !fn_ret_type )
                    error( "return used outside a function" );
                
                auto ret_type = s.e.has_value() ? resolve_expr( s.e.value(), st ) : type_kind::VOID;
                if ( !compatible( fn_ret_type.value(), ret_type ) )
                    error( "incompatible return type in function" );
                break;
            }

            case stmt::if_stmt:
            {
                if ( !s.e.has_value() )
                    error( "if without condition" );
                resolve_expr( s.e.value(), st );
                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                break;
            }
            
            case stmt::for_stmt:
            {   
                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
            
                break;
            }

            case stmt::while_stmt:
            {
                if ( !s.e.has_value() )
                    error( "while statement without expression" );

                resolve_expr( s.e.value(), st );

                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                break;
            }

            case stmt::do_while_stmt:
            {
                if ( !s.e.has_value() )
                    error( "do-while statement without condition" );

                resolve_expr( s.e.value(), st );
                resolve_stmt( s[ 0 ], st );
                break;
            }

            case stmt::cont:
            {    
                if ( !st.within_loop() )
                    error( "continue used outside of a loop" );
                break;
            }

            case stmt::brk:
            {
                if ( !st.within_loop() )
                    error( "break used outside of a loop" );
                break;
            }

            case stmt::block:
            {
                st.create_scope();
                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                st.close_scope();
                break;
            }

            case stmt::var_dclr:
            {
                st.add_var( s.vdecl.name, s.vdecl.type );
                if ( s.e.has_value() )
                    resolve_expr( s.e.value(), st );
                break;
            }

            case stmt::expr_stmt:
            {
                if ( s.e.has_value() )
                    resolve_expr( s.e.value(), st );
                // FIXME: else void ?
                break; 
            }

            default:
                break;
        }

        st.pop_kind();
    }

    void run( ast::program& prog )
    {
        symtab st{};
        st.create_scope(); // global scope

        for ( auto& d : prog.decls )
        {
            std::visit( [ & ]( auto&& arg )
            {
                using T = std::decay_t< decltype( arg ) >;
                if constexpr( std::is_same_v< T, ast::fn_decl > )
                {
                    ast::fn_decl fn = arg;
                    st.add_var( fn.name, fn.sig_type );
                    st.create_scope(); // fn scope;
                    st.push_kind( { .cat = scope_kind::function, .fn_ret_type = fn.sig_type, .fn_name = std::string( fn.name ) } );
                    for ( auto& p : fn.params )
                        st.add_var( p.name, p.type );
                   
                    for ( auto& s : fn.body )
                        resolve_stmt( s, st );

                    st.close_scope();
                    st.pop_kind();
                }
            }, d );
        }

    }
};

}
