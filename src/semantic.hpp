#pragma once

#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string>
#include <vector>

#include "types.hpp"
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

using type = std::variant< fn_signature, prim_type >;

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
        
        fn_signature sig;
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
    
    void add_var( std::string_view name, prim_type pt )
    {
        atom a = get( name );
        if ( scopes.back().contains( a.idx ) )
            error( "re-definition of identifier: ", name );

        map[ std::string( name ) ] = a.idx;
        scopes.back().emplace( a.idx, pt );
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

    std::optional< prim_type > enclosing_fn_ret_type() const
    {
        for ( int i = kinds.size() - 1; i >= 0; -- i  )
        {
            scope_kind sk = kinds[ i ];
            if ( sk.cat == scope_kind::function )
                return sk.sig.ret_type;
        }

        return {};
    }
};

struct semantic
{
    using expr = ast::expr;
    using stmt = ast::stmt;
    using scope_kind = symtab::scope_kind;

    bool resolve_op( type lhs, type rhs, ast::op_kind op ) { return true; }
    
    bool compatible( type to, type from ) { return true; }

    // TODO: assign types to expressions
    // TODO: why are we even doing type return here ? shouldn't it just be primitive_type
    type resolve_expr( ast::expr& e, symtab& st )
    {
        switch ( e.cat )
        {
            case expr::num_lit:
            {
                return prim_type::INT;
            }

            case expr::bool_lit:
            {
                return prim_type::BOOL;
            }

            case expr::identifier:
            {
                auto v = st.find_var( std::string( e.id ) );
                if ( !v )
                    error( e.src_loc, "variable used before definition", e.id );
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
                    error( e.src_loc, "invalid types in operands in operation", e.op );
                return prim_type::VOID;
            }

            case expr::assign:
            {
                auto lhs = resolve_expr( e[ 0 ], st ); 
                auto rhs = resolve_expr( e[ 1 ], st );
                if ( !compatible( lhs, rhs ) )
                    error( e.src_loc, "invalid types in assignment" );
                return rhs;
            }

            case expr::call:
            {
                std::string id = std::string( e.subs[ 0 ].id );
                auto callee = st.find_var( id );
                if ( !callee )
                    error( e.src_loc, "unknown identifier: ", e.id );
                
                if ( !std::holds_alternative< fn_signature >( callee.value() ) )
                    error( e.src_loc, "expected a function call but insead got a primitive type " );
                
                fn_signature fsig = std::get< fn_signature >( callee.value() );
                int actuals_amount = e.subs.size() - 1;
                if ( actuals_amount != fsig.param_types.size() )
                    error( e.src_loc, id, " expected ", fsig.param_types.size(), " arguments, but got ", actuals_amount, " instead" );

                for ( int i = 1; i < e.subs.size(); ++ i )
                    if ( !compatible( fsig.param_types[ i - 1 ], resolve_expr( e.subs[ i ], st ) ) )
                        error( e.src_loc, "invalid types in function call" );

                return e.subs[ 0 ].type;
            }

            default:
                error( "should not reach here" );
                break;
        }
        
        return prim_type::VOID;
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
                    error( s.src_loc, "return used outside a function" );
                
                auto ret_type = s.e.has_value() ? resolve_expr( s.e.value(), st ) : prim_type::VOID;
                if ( !compatible( fn_ret_type.value(), ret_type ) )
                    error( s.src_loc, "incompatible return type in function" );
                break;
            }

            case stmt::if_stmt:
            {
                if ( !s.e.has_value() )
                    error( s.src_loc, "if without condition" );
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
                    error( s.src_loc, "while statement without expression" );

                resolve_expr( s.e.value(), st );

                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                break;
            }

            case stmt::do_while_stmt:
            {
                if ( !s.e.has_value() )
                    error( s.src_loc, "do-while statement without condition" );

                auto cond_t = resolve_expr( s.e.value(), st );
                // TODO
                // if ( !conditional_type( cond_t ) )
                //     error( "expression cannot be used as a condition" );
                resolve_stmt( s[ 0 ], st );
                break;
            }

            case stmt::cont:
            {    
                if ( !st.within_loop() )
                    error( s.src_loc, "continue used outside of a loop" );
                break;
            }

            case stmt::brk:
            {
                if ( !st.within_loop() )
                    error( s.src_loc, "break used outside of a loop" );
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
                {
                    type rhs_type = resolve_expr( s.e.value(), st );
                    if ( !compatible( s.vdecl.type, rhs_type ) )
                        error( s.src_loc, "Incompatible sides of assignment" );
                }
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
                    st.add_var( fn.name, fn.sig.ret_type );
                    st.create_scope(); // fn scope;
                    st.push_kind( { .cat = scope_kind::function, .sig = fn.sig, .fn_name = std::string( fn.name ) } );
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
