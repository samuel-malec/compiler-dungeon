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

struct symtab
{
    struct scope
    {
        enum cat_t
        {
            global,
            function,
            loop,
            block,
        } cat;
        
        function_type sig;
        std::string fn_name;
        std::map< uint32_t, type > symbols;
    };
    
    std::map< std::string, uint32_t, std::less<> > map;
    std::vector< scope > scopes;

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

    void push_scope( scope scope ) 
    {
        scopes.push_back( std::move( scope ) );
    }

    void pop_scope() { scopes.pop_back(); }

    void add_sym( std::string_view name, type typ )
    {
        atom a = get( name );
        if ( scopes.back().symbols.contains( a.idx ) )
            error( "redeclaration of identifier: ", name, "with type", typ );

        map[ std::string( name ) ] = a.idx;
        scopes.back().symbols.emplace( a.idx, typ );
    }

    std::optional< type > find_sym( std::string name )
    {
        atom a = get( name );
        for ( int i = scopes.size() - 1; i >= 0; -- i )
        {
            auto it = scopes[ i ].symbols.find( a.idx );
            if ( it == scopes[ i ].symbols.end() )
                continue;
            
            return it->second;
        }

        return {};
    }
    
    bool within_loop() const
    {
        for ( int i = scopes.size() - 1; i >= 0; -- i )
        {
            if ( scopes[ i ].cat == scope::cat_t::block )
                continue;
            if ( scopes[ i ].cat == scope::cat_t::loop )
                return true;
        }
        return false;
    }

    std::optional< scope > enclosing_fn_scope() const
    {
        for ( int i = scopes.size() - 1; i >= 0; -- i  )
        {
            if ( scopes[ i ].cat == scope::cat_t::function )
                return scopes[ i ];
        }

        return {};
    }
};

struct semantic_analyzer
{
    using expr = ast::expr;
    using stmt = ast::stmt;

    type resolve_unary( ast::expr& e )
    {
        if ( !e[ 0 ].typ.is_primitive() )
            error( e[ 0 ].src_loc, "unexpected type in operation", e[ 0 ].typ );

        if ( e[ 0 ].typ.as_primitive() == prim_type::VOID )
            error( e[ 0 ].src_loc, "invalid operand to unary expression", e[ 0 ].typ );
        
        if ( e[ 0 ].typ.as_primitive() == prim_type::BOOL && e.op != ast::op_kind::NOT )
            error( e[ 0 ].src_loc, "invalid operand to unary expression", e[ 0 ].typ );

        if ( e[ 0 ].typ.as_primitive() == prim_type::INT && ( e.op != ast::op_kind::ADD && e.op != ast::op_kind::SUB ) )
            error( e[ 0 ].src_loc, "invalid operand to unary expression", e[ 0 ].typ );

        e.typ = e[ 0 ].typ;

        return e.typ;
    }

    type resolve_binary( ast::expr& e ) 
    {
        if ( !e[ 0 ].typ.is_primitive() || !e[ 1 ].typ.is_primitive() )
            error( e.src_loc, "invalid operands to binary expression: ", e[ 0 ].typ, e[ 1 ].typ );

        if ( e[ 0 ].typ.as_primitive() == prim_type::VOID || e[ 1 ].typ.as_primitive() == prim_type::VOID )
            error( e.src_loc, "invalid operands to binary expression: ", e[ 0 ].typ, e[ 1 ].typ );
        
        if ( is_bool_op( e.op ) )
        {
            if ( e[ 0 ].typ.as_primitive() != prim_type::BOOL || e[ 1 ].typ.as_primitive() != BOOL )
                error( "invalid operands to binary expression: ", e[ 0 ].typ, e[ 1 ].typ );
            e.typ = type{ .data = prim_type::BOOL };
        }

        if ( is_rel_op( e.op ) )
        {
            if ( e[ 0 ].typ.as_primitive() != e[ 1 ].typ.as_primitive() )
                error( "invalid operands to binary expression: ", e[ 0 ].typ, e[ 1 ].typ );

            e.typ = type{ .data = prim_type::BOOL } ;
        }

        if ( is_numerical_op( e.op ) )
        {
            if ( e[ 0 ].typ.as_primitive() != prim_type::INT || e[ 1 ].typ.as_primitive() != INT )
                error( "invalid operands to binary expression: ", e[ 0 ].typ, e[ 1 ].typ );
            e.typ = type{ .data = prim_type::INT };
        }

        return e.typ;
    }

    bool compatible( type to, type from ) 
    {
        if ( ( to.is_primitive() && to.as_primitive() == prim_type::VOID ) ||
             ( from.is_primitive() && from.as_primitive() == prim_type::VOID ) ) 
            return false;
        
        return to == from;
    }

    bool check_call( symtab& st, std::string_view id, ast::expr& callee, function_type& fn_sig ) 
    {  
        int actuals_amount = callee.subs.size() - 1;
        if ( actuals_amount != fn_sig.params.size() )
            error( callee.src_loc, id, " expected ", fn_sig.params.size(), " arguments, but got ", actuals_amount, " instead" );

        for ( int i = 1; i < callee.subs.size(); ++ i )
            if ( !compatible( type{ .data = fn_sig.params[ i - 1 ] }, resolve_expr( callee.subs[ i ], st ) ) )
                error( callee.src_loc, "invalid types in function call, expected ", fn_sig.params[ i - 1 ], ", but got ", callee.subs[ i ].typ, " instead" );

        return true;
    }

    bool is_assignable( ast::expr& e ) 
    {
        return e.val_kind == ast::expr::val_kind_t::lvalue;
    }

    bool check_condition( ast::expr& e ) 
    {
        return e.typ.is_primitive() && e.typ.as_primitive() == prim_type::BOOL;
    }

    type resolve_expr( ast::expr& e, symtab& st )
    {
        switch ( e.cat )
        {
            case expr::num_lit:
            {
                e.typ = type{ .data = INT };
                break;
            }

            case expr::bool_lit:
            {
                e.typ = type{ .data = BOOL };
                break;
            }

            case expr::identifier:
            {
                auto v = st.find_sym( std::string( e.id ) );
                if ( !v )
                    error( e.src_loc, e.id, "undeclared ", e.id );
                e.typ = v.value();
                break;
            }

            case expr::unary:
            {
                resolve_expr( e[ 0 ], st );
                e.typ = resolve_unary( e );
                break;
            }

            case expr::binary:
            case expr::relational:
            {
                resolve_expr( e[ 0 ], st );
                resolve_expr( e[ 1 ], st );
                e.typ = resolve_binary( e );
                break;
            }

            case expr::assign:
            {
                auto lhs = resolve_expr( e[ 0 ], st );
                if ( !is_assignable( e[ 0 ] ) )
                    error( e[ 0 ].src_loc, "invalid left side of an assignment, expected an lvalue" );

                auto rhs = resolve_expr( e[ 1 ], st );
                if ( !compatible( lhs, rhs ) )
                    error( e.src_loc, "invalid types in assignment" );
                
                e.typ = rhs;
                break;
            }

            case expr::call:
            {
                std::string id = std::string( e.subs[ 0 ].id );
                auto callee = st.find_sym( id );
                if ( !callee )
                    error( e.src_loc, "unknown identifier: ", e.id );
                
                if ( !callee.value().is_function() )
                    error( e.src_loc, "expected a function call but insead got: ", callee.value().describe() );
                
                function_type fn_sig = callee.value().as_function();
                check_call( st, id, e, fn_sig );
                e.typ = type{ .data = fn_sig.ret_type };
                break;
            }

            default:
                error( "should not reach here" );
                break;
        }
        return e.typ;
    }

    void resolve_stmt( ast::stmt& s, symtab& st )
    {
        using scope_cat = symtab::scope::cat_t;

        switch ( s.cat )
        {
            case stmt::ret:
            {
                auto fn_scope = st.enclosing_fn_scope();
                if ( !fn_scope )
                    error( s.src_loc, "return used outside a function" );
                
                auto ret_type = type{ .data = fn_scope.value().sig.ret_type };
                auto expr_type = s.e.has_value() ? resolve_expr( s.e.value(), st ) : type{ .data = prim_type::VOID };
                if ( !compatible( ret_type , expr_type ) )
                    error( s.src_loc, "expresion of type: ", expr_type, " is incompatible with return type:", ret_type );
                break;
            }

            case stmt::if_stmt:
            {
                if ( !s.e.has_value() )
                    error( s.src_loc, "if without condition" );
                
                resolve_expr( s.e.value(), st );
                if ( !check_condition( s.e.value() ) )
                    error( s.src_loc, "expected a condition" );

                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                break;
            }
            
            case stmt::for_stmt:
            {
                // s[ 0 ] --> init
                // s[ 1 ] --> cond
                // s[ 2 ] --> update
                // s[ 3 ] --> body
                st.push_scope( { .cat = scope_cat::loop } );
                resolve_stmt( s[ 0 ], st );
                resolve_stmt( s[ 1 ], st );
                if ( !check_condition( s[ 1 ].e.value() ) )
                    error( s[ 1 ].src_loc, "expected a condition" );
                resolve_stmt( s[ 2 ], st );

                for ( int i = 3; i < s.subs.size(); ++ i )
                    resolve_stmt( s.subs[ i ], st );
                
                break;
            }

            case stmt::while_stmt:
            {
                if ( !s.e.has_value() )
                    error( s.src_loc, "while statement without expression" );

                resolve_expr( s.e.value(), st );
                if ( !check_condition( s.e.value() ) ) 
                    error( s.src_loc, "expected a condition" );
                
                st.push_scope( { .cat = scope_cat::loop } );
                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                break;
            }
            
            case stmt::do_while_stmt:
            {
                if ( !s.e.has_value() )
                    error( s.src_loc, "do-while statement without condition" );

                resolve_expr( s.e.value(), st );
                if ( !check_condition( s.e.value() ) )
                    error( s.src_loc, "expected a condition" );

                st.push_scope( { .cat = scope_cat::loop } );
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
                st.push_scope( { .cat = scope_cat::block } );
                for ( auto& sub : s.subs )
                    resolve_stmt( sub, st );
                st.pop_scope();
                break;
            }

            case stmt::var_dclr:
            {
                st.add_sym( s.vdecl.name, s.vdecl.typ );
                if ( s.e.has_value() )
                { 
                    type rhs_type = resolve_expr( s.e.value(), st );
                    if ( !compatible( s.vdecl.typ, rhs_type ) )
                        error( s.src_loc, "incompatible sides of assignment", s.vdecl.typ, rhs_type );
                }
                break;
            }

            case stmt::expr_stmt:
            {
                if ( s.e.has_value() )
                    resolve_expr( s.e.value(), st );
                else
                    error( s.src_loc, "should not reach here, !s.e.has_value() " );
                
                break; 
            }

            default:
                break;
        }
    }

    void run( ast::program& prog )
    {
        using scope_cat = symtab::scope::cat_t;

        symtab st{};
        st.push_scope( { .cat = scope_cat::global } );

        for ( auto& d : prog.decls )
        {
            std::visit( [ & ]( auto&& arg )
            {
                using T = std::decay_t< decltype( arg ) >;
                if constexpr( std::is_same_v< T, ast::fn_decl > )
                {
                    ast::fn_decl fn = arg;
                    st.add_sym( std::string( fn.name ), type{ .data = fn.sig } );
                    st.push_scope( { .cat = scope_cat::function, .sig = fn.sig, .fn_name = std::string( fn.name ) } );

                    for ( auto& p : fn.params )
                        st.add_sym( p.name, p.typ );
                   
                    for ( auto& s : fn.body )
                        resolve_stmt( s, st );

                    st.pop_scope();
                }
            }, d );
        }
    }
};

}
