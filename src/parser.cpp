#include <iostream>

#include "parser.hpp"
// TODO: do we really want optional here ? 

namespace dungeon
{   
    using cat = token::cat_t;
    using expr = ast::expr;
    using stmt = ast::stmt;
    using decl = ast::decl;
    using var_decl = ast::var_decl;
    using program = ast::program;

    std::optional< decl > parser::parse_toplevel_decl()
    {
        // TODO: structs and enums later ( hopefully )
        std::optional< decl > res{};
        
        if ( ( res = parse_fn_decl() ) )
            return res;

        return {};
    }

    std::optional< decl > parser::parse_fn_decl()
    {
        auto t = require( cat::keyword );
        auto tk = type_from_str( t.data );
        if ( tk == ast::UNKNOWN )
            error( t, "Expected a type" );
        
        auto id = require( cat::ident );
        require( cat::punct, "(" );

        decl res{ .cat = decl::fn_decl, .type = tk, .name = id.data };

        res.vars = std::move( parse_var_decl_list() );

        require( cat::punct, ")" );
        
        auto body = parse_stmt();
        if ( !body )
            return {};
        
        res.body = std::move( body.value() );
        return res;
    }
    
    std::vector< var_decl > parser::parse_var_decl_list()
    {
        std::vector< var_decl > var_decls{};

        while ( !match( cat::punct, ")" ) )
        {
            if ( !var_decls.empty() )
                require( cat::punct, "," );

            auto t = require( cat::keyword );
            auto tk = type_from_str( t.data );
            if ( tk == ast::UNKNOWN )
                error( t, "Expected a type" );
            
            auto id = require( cat::ident );
            var_decl tmp{ .name = id.data, .type = tk };
            var_decls.push_back( tmp );
        }

        return var_decls;
    }

    std::optional< stmt > parser::parse_stmt()
    {
        std::cout << "parse stmt\n";
        std::optional< stmt > res{};
        if ( ( res = parse_loop_stmt() ) 
            || ( res = parse_block() ) 
            || ( res = parse_if() )
            || ( res = parse_expr_stmt() ) 
            || ( res = parse_ret() )
            || ( res = parse_control_stmt() )
            || ( res = parse_var_decl() )
            || ( res = parse_assign() ) )
            return res;
        
        return {};
    }

    std::optional< stmt > parser::parse_block()
    {
        std::cout << "parse block\n";
        if ( !match( cat::punct, "{" ) )
            return {};

        fetch();
        stmt res{ .cat = stmt::block };

        while ( !match( cat::punct, "}" ) )
        {
            auto s = parse_stmt();
            if ( !s )
                return {};

            res.subs.push_back( s.value() );
        }

        return res;
    }
    
    std::optional< stmt > parser::parse_loop_stmt() 
    {
        std::optional< stmt > res{};
        if ( ( res = parse_for() )
            || ( res = parse_while() )
            || ( res = parse_do_while() ) )
            return res;

        return {}; 
    }

    std::optional< stmt > parser::parse_for() 
    {
        if ( !match( cat::keyword, "for" ) )
            return {};
        
        require( cat::punct, "(" );
        // todo


        return {}; 
    }

    std::optional< stmt > parser::parse_while() 
    {
        if ( !match( cat::keyword, "while" ) )
            return {};
        
        require( cat::punct, "(" );

        // todo
        return {};
    } 

    std::optional < stmt > parser::parse_do_while() 
    {
        if ( !match( cat::keyword, "do" ) )
            return {};
        

        return {};
    }

    std::optional< stmt > parser::parse_if() 
    {
        if ( !match( cat::keyword, "if" ) )
            return {};
        
        require( cat::punct, "(" );
        auto cond = parse_expr();
        if ( !cond )
            return {};

        require( cat::punct, ")" );

        auto then_body = parse_stmt();
        if ( !then_body )
            return {};

        // else block
        stmt res{ .cat = stmt::if_stmt, .e = cond.value() };
        res.subs.push_back( std::move( then_body.value() ) );
        return res;
    }

    std::optional< stmt > parser::parse_expr_stmt()
    {
        auto e = parse_expr();
        if ( !e )
            return {};
            
        return stmt{ .cat = stmt::expr_stmt, .e = e.value() };
    }
    
    std::optional< stmt > parser::parse_ret() 
    {
        if ( !match( cat::keyword, "return" ) )
            return {};

        fetch();
        auto e = parse_expr();
        if ( !e )
            return {};

        require( cat::punct, ";" );
        
        return stmt{ .cat = stmt::ret, .e = e.value() };
    }

    std::optional< stmt > parser::parse_control_stmt()
    {
        if ( match( cat::keyword, "break" ) )
        {
            require( cat::punct, ";" );
            return stmt{ .cat = stmt::brk };
        }

        if ( match( cat::keyword, "continue" ) )
        {
            require( cat::punct, ";" );
            return stmt{ .cat = stmt::cont };
        }

        return {};
    }

    std::optional< stmt > parser::parse_var_decl()
    {
        if ( current.cat == cat::invalid )
            return {};

        auto tk = type_from_str( current.data );
        if ( tk == type_k::UNKNOWN )
            return {};
        
        auto id = require( cat::ident );
        fetch();
        
        var_decl vdecl{ .name = id.data, .type = tk };
        if ( match( cat::punct, ";") )
            return stmt{ .cat = stmt::var_dclr, .vdecl = vdecl };
    
        if ( !match( cat::punct, "=" ) )
            return {};
        
        fetch();
        auto e = parse_expr();
        if ( !e )
            return {};
        
        vdecl.e = e.value();
        return stmt{ .cat = stmt::var_dclr, .vdecl = vdecl };
    }

    std::optional< stmt > parser::parse_assign() 
    { 
        if ( !match( cat::ident ) )
            return {};
        
        require( cat::punct, "=" );
        auto e = parse_expr();
        if ( !e )
            return {};

        return stmt{ .cat = stmt::var_assign, .e = e.value() };
    }

    std::optional< expr > parser::parse_expr() 
    { 
        std::cout << " TODO ";
        assert( false );
        return {}; 
    }

}
