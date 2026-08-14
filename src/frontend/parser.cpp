#include <charconv>

#include "parser.hpp"

namespace dungeon
{   
    using cat = token::cat_t;
    using expr = ast::expr;
    using stmt = ast::stmt;
    using toplevel = ast::toplevel;
    using fn_decl = ast::fn_decl;
    using var_decl = ast::var_decl;
    using module = ast::module;
    using program = ast::program;

    std::optional< expr > parser::parse_primary()
    {
        if ( match( cat::punct, "(" ) )
        {
            fetch();
            auto e = parse_expr();
            require( cat::punct, ")" );
            return e;
        }

        if ( match( cat::number ) )
        {
            auto tok = fetch();
            uint64_t n{};
            auto [ p, ec ] = std::from_chars( tok.data.data(), tok.data.data() + tok.data.size(), n );
            if ( ec != std::errc() || p != tok.data.data() + tok.data.size() )
                diag::error( tok, "Invalid numeric literal" );

            return ast::expr{ .src_loc = tok.loc, .data = ast::num_lit_data{ .value = n } };
        }

        if ( auto t = match_any( cat::keyword, "true", "false" ) )
        {
            auto tok = fetch();
            bool value = t.value().data == "true";
            return ast::expr{ .src_loc = tok.loc, .data = ast::bool_lit_data{ .value = value } };
        }

        if ( match( cat::ident ) )
        {
            auto tok = fetch();
            return ast::expr{ .src_loc = tok.loc, .data = ast::identifier_data{ .id = tok.data } };
        }

        return {};
    }

    // TODO
    std::optional< expr > parser::parse_postfix()
    {
        auto e = parse_primary();
        if ( !e )
            return {};

        while ( match( cat::punct, "(" ) )
        {
            fetch();

            ast::call_data cd{};
            ast::identifier_data eid;
            if ( eid = std::get_if< ast::identifier_data >( *e->data ) )
                diag::error("Expected an identifier as a callee" );

            cd.callee = make_expr( std::move( *e ) );
            if ( !match( cat::punct, ")" ) )
            {
                while ( true )
                {
                    auto arg = parse_expr();
                    if ( !arg )
                        diag::error( "Expected function argument" );

                    cd.args.push_back( make_expr( std::move( arg.value() ) ) );
                    if ( !match( cat::punct, "," ) )
                        break;
                    fetch();
                }
            }

            require( cat::punct, ")" );
            e = expr{ .src_loc = *e->src_loc, .data = std::move( cd ) };
        }

        return e;
    }

    std::optional< expr > parser::parse_unary()
    {
        if ( auto t = match_any( cat::punct, "!", "-", "+" ) )
        {
            fetch();
            auto rhs = parse_unary();
            if ( !rhs )
                diag::error( "Expected unary operand" );

            ast::unary_data ud{};
            ud.lhs = make_expr( std::move( rhs.value() ) );
            ud.op = op_kind_from_str( t->data );
            return expr{ .src_loc = t->loc, .data = std::move( ud ) };
        }

        return parse_postfix();
    }

    std::optional< expr > parser::parse_factor()
    {
        auto e = parse_unary();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "/", "*", "%" ) )
        {
            fetch();
            auto rhs = parse_unary();
            if ( !rhs )
                diag::error( "Expected rhs for multiplicative expression" );

            e = std::move( make_binary( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data ) ) );
        }

        return e;
    }

    std::optional< expr > parser::parse_term()
    {
        auto e = parse_factor();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "+", "-" ) )
        {
            fetch();
            auto rhs = parse_factor();
            if ( !rhs )
                diag::error( "Expected rhs for additive expression" );

            e = std::move( make_binary( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data ) ) );
        }

        return e;
    }

    std::optional< expr > parser::parse_shift()
    {
        auto e = parse_term();
        if ( !e )
            return {};

        while( auto t = match_any( cat::punct, ">>", "<<" ) )
        {
            fetch();
            auto rhs = parse_term();
            if ( !rhs )
                diag::error( "Expected rhs for shift expression" );

            e = std::move( make_binary( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data ) ) );
        }

        return e;
    }
    
    std::optional< expr > parser::parse_comparison()
    {
        auto e = parse_shift();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "<", "<=", ">", ">=" ) )
        {
            fetch();
            auto rhs = parse_shift();
            if ( !rhs )
                diag::error( "Expected rhs for comparison expression" );

            e = std::move( make_relational( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data ) ) );
        }

        return e;
    }
 
    std::optional< expr > parser::parse_equality()
    {
        auto e = parse_comparison();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "==", "!=" ) )
        {
            fetch();
            auto rhs = parse_comparison();
            if ( !rhs )
                diag::error( "Expected rhs for equality expression" );

            e = std::move( make_relational( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data ) ) );
        }

        return e;
    }

    std::optional< expr > parser::parse_assignment()
    {
        auto e = parse_equality();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "=", "+=", "-=", "*=", "<<=", ">>=" ) )
        {
            fetch();
            auto rhs = parse_assignment();
            if ( !rhs )
                diag::error( "Expected rhs for assignment expression" );

            ast::assign_data ad{};
            if ( t->data == "==" )
            {
                if ( auto i = std::get_if< ast::assign_data >( *e->data ) )
                    diag::error( "Lhs of an assignment must be an identifier");
                // TODO: parse equality
            }
            else
            {
                // make_compound_assignment( std::move( e ), std::move( rhs.value() ), std::move( ad ) );
            }

            ad.id = t->data;
            ad.val = make_expr( std::move( rhs.value() ) );
            e = expr{ .src_loc = e->src_loc, .data = std::move( ad ) };
        }

        return e;    
    }

    std::optional< expr > parser::parse_and()
    {
        auto e = parse_assignment();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "&&" ) )
        {
            fetch();
            auto rhs = parse_assignment();
            if ( !rhs )
                diag::error( "Expected rhs for assignment expression" );

            e = std::move( make_binary( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data )) );
        }

        return e;  
    }

    std::optional< expr > parser::parse_or()
    {
        auto e = parse_and();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "||" ) )
        {
            fetch();
            auto rhs = parse_assignment();
            if ( !rhs )
                diag::error( "Expected rhs for assignment expression" );

            e = std::move( make_binary( std::move( e.value() ), std::move( rhs.value() ), op_kind_from_str( t->data ) ) );
        }

        return e;  
    }

    std::optional< expr > parser::parse_expr() 
    { 
        return parse_or();
    }

    std::optional< stmt > parser::parse_expr_stmt()
    {
        auto e = parse_expr();
        if ( !e )
            return {};

        require( cat::punct, ";" );
        return stmt{ .loc = e->src_loc, .data = ast::expr_stmt_data{ .expr = make_expr( std::move( *e ) ) } };
    }

    std::optional< stmt > parser::parse_block()
    {
        if ( !match( cat::punct, "{" ) )
            return {};

        fetch();
        ast::block_data bd{};

        while ( !match( cat::punct, "}" ) )
        {
            auto s = parse_stmt();
            if ( !s )
                diag::error( "Parsing statement inside a block: " );
            bd.stmts.push_back( make_stmt( std::move( *s ) ) );
        }

        fetch();
        return  stmt{ .data = std::move( bd ) };
    }
    
    std::optional< stmt > parser::parse_if() 
    {
        if ( !match( cat::keyword, "if" ) )
            return {};

        fetch();
        require( cat::punct, "(" );

        auto cond = parse_expr();
        if ( !cond )
            diag::error( "Expected condition after if" );

        require( cat::punct, ")" );
        auto then_body = parse_stmt();
        if ( !then_body )
            diag::error( "Empty body inside if block" );

        stmt res{ if_stmt, .e = cond.value() };
        res.subs.push_back( std::move( then_body.value() ) );
        if ( !match( cat::keyword, "else" ) )
            return res;

        fetch();
        auto else_body = parse_stmt();
        if ( !else_body )
            diag::error( "Empty else body" );

        res.subs.push_back( std::move( else_body.value() ) );
        return res;
    }
  
    std::optional< stmt > parser::parse_ret() 
    {
        if ( !match( cat::keyword, "return" ) )
            return {};

        fetch();
        auto res = stmt{ .cat = stmt::ret };
        
        if ( match( cat::punct, ";" ) )
        {
            fetch();
            return res;
        }

        auto e = parse_expr();
        if ( !e )
            diag::error( "Expected expression" );
            
        res.e = e.value();
        require( cat::punct, ";" );
        return res;
    }

    std::optional< stmt > parser::parse_control_stmt()
    {
        if ( match( cat::keyword, "break" ) )
        {
            fetch();
            require( cat::punct, ";" );
            return stmt{ .cat = stmt::brk };
        }

        if ( match( cat::keyword, "continue" ) )
        {
            fetch();
            require( cat::punct, ";" );
            return stmt{ .cat = stmt::cont };
        }

        return {};
    }

    std::optional< stmt > parser::parse_for() 
    {
        if ( !match( cat::keyword, "for" ) )
            return {};

        fetch();
        require( cat::punct, "(" );
        stmt init{};

        if ( !match( cat::punct, ";" ) )
        {
            if ( auto v = parse_var_decl() )
                init = std::move( v.value() );
            else
            {
                auto ie = parse_expr();
                if ( !ie )
                    diag::error( "Invalid for loop initializer" );

                init = stmt{ .cat = stmt::expr_stmt, .e = ie.value() };
                require( cat::punct, ";" );
            }
        }
        else
            fetch();
        
        auto cond_expr = make_expr_node( expr::bool_lit, type{ .data = BOOL } );
        cond_expr.val = true;
        cond_expr.val_kind = expr::rvalue;
        stmt cond{ .cat = stmt::expr_stmt, .e = cond_expr };
        
        if ( !match( cat::punct, ";" ) )
        {
            auto ce = parse_expr();
            if ( !ce )
                diag::error( "Invalid for loop condition" );

            cond.e = ce.value();
        }

        require( cat::punct, ";" );
        stmt update{ .cat = stmt::expr_stmt };
        if ( !match( cat::punct, ")" ) )
        {
            auto ue = parse_expr();
            if ( !ue )
                error( "Invalid for loop update" );
            update.e = ue.value(); 
        }

        require( cat::punct, ")" );
        auto body = parse_stmt();
        if ( !body )
            diag::error( "Empty body in for loop" );

        stmt res{ .cat = stmt::for_stmt };
        res.subs.push_back( std::move( init ) );
        res.subs.push_back( std::move( cond ) );
        res.subs.push_back( std::move( update ) );
        res.subs.push_back( std::move( body.value() ) );
        return res;
    }

    std::optional< stmt > parser::parse_while() 
    {
        if ( !match( cat::keyword, "while" ) )
            return {};
        fetch();
        
        require( cat::punct, "(" );
        auto e = parse_expr();
        if ( !e )
            diag::error( "Empty condition in while" );

        require( cat::punct, ")" );
        auto body = parse_stmt();
        if ( !body )
            diag::error( "Empty body in while" );

        stmt res{ .cat = stmt::while_stmt, .e = e.value() };
        res.subs.push_back( std::move( body.value() ) );
        return res;
    } 

    std::optional < stmt > parser::parse_do_while() 
    {
        if ( !match( cat::keyword, "do" ) )
            return {};

        fetch();
        auto s = parse_stmt();
        if ( !s )
            diag::error( "Empty body in do_while" );

        require( cat::keyword, "while" );
        require( cat::punct, "(" );
        auto e = parse_expr();
        if ( !e )
            diag::error( "Empty condition in do_while" );

        require( cat::punct, ")" );
        require( cat::punct, ";" );

        stmt res{ .cat = stmt::do_while_stmt, .e = e.value() };
        res.subs.push_back( std::move( s.value() ) );
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
    
    std::vector< var_decl > parser::parse_var_decl_list()
    {
        std::vector< var_decl > var_decls{};

        while ( !match( cat::punct, ")" ) )
        {
            if ( !var_decls.empty() )
                require( cat::punct, "," );

            auto t = require( cat::keyword );
            if ( !tk )
                diag::error( t, "Expected a type" );
            
            auto id = require( cat::ident );
            var_decl tmp{ .name = id.data, .typ = tk.value() };
            var_decls.push_back( tmp );
        }

        return var_decls;
    }

    std::optional< var_decl > parser::parse_var_decl_info()
    {
        if ( !match( cat::keyword ) )
            return {};

        auto tk = type_from_str( peek().data );
        if ( !tk )
            return {};
        
        fetch();
        auto id = require( cat::ident );
        var_decl vdecl{ .name = id.data, .typ = tk.value(), .e = std::nullopt };

        if ( match( cat::punct, ";" ) )
            return vdecl;
    
        if ( !match( cat::punct, "=" ) )
            error( "Unexpected symbol in variable declaration" );
        
        fetch();
        auto e = parse_expr();
        if ( !e )
            return {};

        vdecl.e = e.value();        
        return vdecl;
    }

    std::optional< stmt > parser::parse_var_decl()
    {
        auto vdecl = parse_var_decl_info();
        if ( !vdecl )
            return {};

        require( cat::punct, ";" );
        return stmt{ .cat = stmt::var_dclr, .vdecl = vdecl.value() };
    }

    std::optional< stmt > parser::parse_stmt()
    {
        std::optional< stmt > res{};
        if ( ( res = parse_loop_stmt() )
            || ( res = parse_block() ) 
            || ( res = parse_if() )
            || ( res = parse_ret() )
            || ( res = parse_control_stmt() )
            || ( res = parse_var_decl() )
            || ( res = parse_expr_stmt() ) ) 
            return res;
        
        return {};
    }

    std::optional< fn_decl > parser::parse_fn_decl()
    {
        if ( !match( cat::keyword ) )
            return {};

        auto tk = type_from_str( current.data );
        if ( !tk )
            diag::error( "Expected a type" );
        fetch();

        auto id = require( cat::ident );
        require( cat::punct, "(" );

        fn_decl res{ .name = id.data };
        res.params = std::move( parse_var_decl_list() );
        
        require( cat::punct, ")" );
        require( cat::punct, "{" );

        function_type fn_typ{ .ret_type = tk.value().as_primitive() };
        for ( var_decl& dec : res.params )
            fn_typ.params.push_back( dec.typ.as_primitive() );
        
        res.sig = fn_typ;
        while ( !match( cat::punct, "}" ) )
        {
            auto s = parse_stmt();
            if ( !s )
                error( "Parsing statement in function: ", id );
            res.body.push_back( s.value() );
        }

        fetch();
        return res;
    }

    std::optional< toplevel > parser::parse_toplevel()
    {
        return parse_fn_decl();
    }

    std::optional< ast::module > parse_module()
    {
        ast::module mod{};
        while ( !empty() )
        {
            if ( peek().cat == cat::invalid )
                break;

            std::optional< toplevel > d = parse_toplevel();
            if ( !d )
                error( "Expected a toplevel declaration " );

            mod.toplevel_items.push_back( d.value() );
        }

        return mod;
    }

    // ast::program parse_program()
    // {
    //
    // }
}
