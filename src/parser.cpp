#include <iostream>
#include <charconv>

#include "parser.hpp"

namespace dungeon
{   
    using cat = token::cat_t;
    using expr = ast::expr;
    using stmt = ast::stmt;
    using decl = ast::decl;
    using var_decl = ast::var_decl;
    using fn_decl = ast::fn_decl;
    using struct_decl = ast::struct_decl;
    using enum_decl = ast::enum_decl;
    using program = ast::program;

    static expr make_expr_node( expr::cat_t cat, ast::type_kind type = ast::UNKNOWN )
    {
        return expr{
            .cat = cat,
            .val_kind = expr::rvalue,
            .val = std::monostate{},
            .id = {},
            .subs = {},
            .type = type,
            .op = ast::ADD,
        };
    }

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
                error( tok, "Invalid numeric literal" );

            auto e = make_expr_node( expr::num_lit, ast::UNSIGNED );
            e.val = n;
            e.val_kind = expr::rvalue;
            return e;
        }

        if ( auto t = match_any( cat::keyword, "true", "false" ) )
        {
            fetch();
            bool value = t.value().data == "true";
            auto e = make_expr_node( expr::bool_lit, ast::BOOL );
            e.val = value;
            e.val_kind = expr::rvalue;
            return e;
        }

        if ( match( cat::ident ) )
        {
            auto tok = fetch();
            auto e = make_expr_node( expr::identifier );
            e.id = tok.data;
            e.val = std::string( tok.data );
            e.val_kind = expr::lvalue;
            return e;
        }

        return {};
    }

    std::optional< expr > parser::parse_postfix()
    {
        auto e = parse_primary();
        if ( !e )
            return {};

        while ( match( cat::punct, "(" ) )
        {
            fetch();

            expr call = make_expr_node( expr::call );
            call.id = e->id;
            call.subs.push_back( e.value() );

            if ( !match( cat::punct, ")" ) )
            {
                while ( true )
                {
                    auto arg = parse_expr();
                    if ( !arg )
                        error( "Expected function argument" );

                    call.subs.push_back( arg.value() );

                    if ( !match( cat::punct, "," ) )
                        break;
                    fetch();
                }
            }

            require( cat::punct, ")" );
            e = call;
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
                error( "Expected unary operand" );

            expr out = make_expr_node( expr::unary );
            out.op = op_kind_from_str( t->data );
            out.subs.push_back( rhs.value() );
            return out;
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
                error( "Expected rhs for multiplicative expression" );

            expr tmp = make_expr_node( expr::binary );
            tmp.op = op_kind_from_str( t->data );
            tmp.subs.push_back( e.value() );
            tmp.subs.push_back( rhs.value() );
            e = std::move( tmp );
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
                error( "Expected rhs for additive expression" );

            expr tmp = make_expr_node( expr::binary );
            tmp.op = op_kind_from_str( t->data );
            tmp.subs.push_back( e.value() );
            tmp.subs.push_back( rhs.value() );
            e = std::move( tmp );
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
                error( "Expected rhs for shift expression" );

            expr tmp = make_expr_node( expr::binary );
            tmp.op = op_kind_from_str( t->data );
            tmp.subs.push_back( e.value() );
            tmp.subs.push_back( rhs.value() );
            e = std::move( tmp );
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
                error( "Expected rhs for comparison expression" );

            auto tmp = make_expr_node( expr::relational );
            tmp.op = op_kind_from_str( t->data );
            tmp.subs.push_back( e.value() );
            tmp.subs.push_back( rhs.value() );
            e = std::move( tmp );
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
                error( "Expected rhs for equality expression" );

            auto tmp = make_expr_node( expr::relational );
            tmp.op = op_kind_from_str( t.value().data );
            tmp.subs.push_back( e.value() );
            tmp.subs.push_back( rhs.value() );
            e = tmp;
        }

        return e;
    }

    std::optional< expr > parser::parse_assignment()
    {
        auto e = parse_equality();
        if ( !e )
            return {};

        while ( auto t = match_any( cat::punct, "=" ) )
        {
            fetch();
            auto rhs = parse_assignment();
            if ( !rhs )
                error( "Expected rhs for assignment expression" );

            auto tmp = make_expr_node( expr::assign );
            tmp.op = op_kind_from_str( t->data );
            tmp.subs.push_back( e.value() );
            tmp.subs.push_back( rhs.value() );
            e = std::move( tmp );
        }

        return e;
    }

    std::optional< expr > parser::parse_expr() 
    { 
        return parse_assignment();
    }

    std::optional< stmt > parser::parse_expr_stmt()
    {
        auto e = parse_expr();
        if ( !e )
            return {};

        require( cat::punct, ";" );            
        return stmt{ .cat = stmt::expr_stmt, .e = e.value() };
    }

    std::optional< stmt > parser::parse_block()
    {
        if ( !match( cat::punct, "{" ) )
            return {};

        fetch();
        stmt res{ .cat = stmt::block };

        while ( !match( cat::punct, "}" ) )
        {
            auto s = parse_stmt();
            if ( !s )
                error( "Parsing statement inside a block: " );

            res.subs.push_back( s.value() );
        }

        fetch();
        return res;
    }
    
    std::optional< stmt > parser::parse_if() 
    {
        if ( !match( cat::keyword, "if" ) )
            return {};

        fetch();
        require( cat::punct, "(" );

        auto cond = parse_expr();
        if ( !cond )
            error( "Expected condition after if" );

        require( cat::punct, ")" );
        auto then_body = parse_stmt();
        if ( !then_body )
            error( "Empty body inside if block" );

        stmt res{ .cat = stmt::if_stmt, .e = cond.value() };
        res.subs.push_back( std::move( then_body.value() ) );
        if ( !match( cat::keyword, "else" ) )
            return res;

        fetch();
        auto else_body = parse_stmt();
        if ( !else_body )
            error( "Empty else body" );

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
            error( "Expected expression" );
            
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
                    error( "Invalid for loop initializer" );

                init = stmt{ .cat = stmt::expr_stmt, .e = ie.value() };
                require( cat::punct, ";" );
            }
        }
        else
            fetch();

        stmt cond{ .cat = stmt::expr_stmt };
        if ( !match( cat::punct, ";" ) )
        {
            auto ce = parse_expr();
            if ( !ce )
                error( "Invalid for loop condition" );

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
            error( "Empty body in for loop" );

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
            error( "Empty condition in while" );

        require( cat::punct, ")" );
        auto body = parse_stmt();
        if ( !body )
            error( "Empty body in while" );

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
            error( "Empty body in do_while " );

        require( cat::keyword, "while" );
        require( cat::punct, "(" );
        auto e = parse_expr();
        if ( !e )
            error( "Empty condition in do_while" );

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
            auto tk = type_from_str( t.data );
            if ( tk == ast::UNKNOWN )
                error( t, "Expected a type" );
            
            auto id = require( cat::ident );
            var_decl tmp{ .name = id.data, .type = tk };
            var_decls.push_back( tmp );
        }

        return var_decls;
    }

    std::optional< var_decl > parser::parse_var_decl_info()
    {
        if ( !match( cat::keyword ) )
            return {};

        auto tk = type_from_str( peek().data );
        if ( tk == type_k::UNKNOWN )
            return {};
        
        fetch();
        auto id = require( cat::ident );
        var_decl vdecl{ .name = id.data, .type = tk };
        if ( match( cat::punct, ";" ) )
        {
            fetch();
            return vdecl;
        }
    
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

    std::optional< decl > parser::parse_fn_decl()
    {
        if ( !match( cat::keyword ) )
            return {};

        auto tk = type_from_str( current.data );
        if ( tk == ast::UNKNOWN )
            error( tk, "Expected a type" );
        fetch();

        auto id = require( cat::ident );
        require( cat::punct, "(" );

        fn_decl res{ .sig_type = tk, .name = id.data };
        res.params = std::move( parse_var_decl_list() );
        require( cat::punct, ")" );
        auto body = parse_stmt();
        if ( !body )
            error( "Empty body of function: ", id.data );
        
        res.body = std::move( body.value() );
        return res;
    }

    std::optional< decl > parser::parse_toplevel_decl()
    {
        // TODO: structs and enums later ( hopefully )
        return parse_fn_decl();
    }

    void parser::pad( int depth )
    {
        for ( int i = 0; i < depth * 2; ++ i )
            std::cout << " ";
    }

    // TODO: the printing now is kinda retarded because we can have semantically invalid program parsed
    // Currently this is only for debugging purposes, it will ( hopefully ) get much cleaner once we run semantic analysis
    // and restrict ourselves only to sematically correct programs
    void parser::print_expr( expr& e, int depth )
    {
        pad( depth );
        switch ( e.cat )
        {
            case expr::num_lit:
                std::cout << "[ num_lit ] " << std::get< uint64_t >( e.val );
                break;
            case expr::bool_lit:
                std::cout << "[ bool_lit ] " << std::get< bool >( e.val );
                break;
            case expr::identifier:
                std::cout << "[ id ] " << e.id;
                break;
            case expr::unary:
                std::cout << "[ unary " << e.op << " ]\n";
                print_expr( e[ 0 ], depth + 1 );
                return;
            case expr::binary:
                std::cout << "[ binary " << e.op << " ]\n";
                print_expr( e[ 0 ], depth + 1 );
                print_expr( e[ 1 ], depth + 1 );
                return;
            case expr::relational:
                std::cout << "[ rel " << e.op << " ]\n";
                print_expr( e[ 0 ], depth + 1 );
                print_expr( e[ 1 ], depth + 1 );
                return;
            case expr::assign:
                std::cout << "[ assign ]\n";
                print_expr( e[ 0 ], depth + 1 );
                print_expr( e[ 1 ], depth + 1 );
                return;
            case expr::call:
                std::cout << "[ call ]\n";
                print_expr( e.subs[ 0 ], depth + 1 );
                pad( depth + 1 );
                std::cout << "[ params ]\n";
                for ( int i = 1; i < e.subs.size(); ++ i )
                    print_expr( e.subs[ i ], depth + 2 );
                return;

            default:
                break;
        }
        std::cout << '\n';
    }

    void parser::print_stmt( stmt& s, int depth )
    {
        pad( depth );

        switch ( s.cat )
        {
            case stmt::ret:
                // FIXME: this can be nullopt and it is totally ok, fix maybe in expr
                std::cout << "[ ret ]\n";
                if  ( s.e.has_value() )
                    print_expr( s.e.value(), depth );
                else
                    std::cout << " {}";
                break;
            
            case stmt::if_stmt:
                std::cout << "[ if_stmt ]\n";
                pad( depth + 1 );
                std::cout << "[ cond ]\n";
                if  ( s.e.has_value() )
                    print_expr( s.e.value(), depth + 2 );
                else
                    std::cout << "{}";
                
                print_stmt( s[ 0 ], depth + 1 );
                if ( s.subs.size() > 1 )
                {
                    pad( depth );
                    std::cout << "[ else ]\n";
                    print_stmt( s[ 1 ], depth + 1 );
                }
                return;
            
            case stmt::for_stmt:
                std::cout << "[ for_stmt ]\n";
                for ( auto& se : s.subs )
                    print_stmt( se, depth + 1 );
                return;

            case stmt::while_stmt:
                std::cout << "[ while_stmt ]\n";
                pad( depth );
                std::cout << "[ cond ]\n";
                if ( !s.e.has_value() )
                    std::cout << " {} ";
                else
                    print_expr( s.e.value(), depth );

                std::cout << "\n";
                print_stmt( s[ 0 ], depth + 1 );
                return;

            case stmt::do_while_stmt:
                std::cout << " [ do_while ]\n";
                print_stmt( s[ 0 ], depth + 1 );
                pad( depth );
                std::cout << "[ cond ]\n";
                if ( !s.e.has_value() )
                    std::cout << " {} ";
                else
                    print_expr( s.e.value(), depth );
                return;

            case stmt::cont:
                std::cout << "[ continue ]";
                break;

            case stmt::brk:
                std::cout << "[ break ]";
                break;

            case stmt::block:
                std::cout << "[ block ]\n";
                for ( auto& se : s.subs )
                    print_stmt( se, depth + 1 );
                return;

            case stmt::var_dclr:
                std::cout << "[ var_decl ]\n";
                pad( depth + 1 );
                std::cout << "[ var ] " << s.vdecl.type << " " << s.vdecl.name << '\n';
                if ( s.vdecl.e.has_value() )
                    print_expr( s.vdecl.e.value(), depth + 1 );
                return;

            case stmt::expr_stmt:
                std::cout << "[ expr_stmt ]\n";
                if ( !s.e.has_value() )
                    std::cout << " {} ";
                else
                    print_expr( s.e.value(), depth + 1 );
                return;

            default:
                break;
        }

        std::cout << "\n";
    }

    void parser::print_ast( program& prog )
    {
        for ( auto& decl : prog.decls )
        {
            std::visit( [ this ]( auto&& arg )
            {
                using T = std::decay_t< decltype( arg ) >;
                if constexpr ( std::is_same_v< T, fn_decl > )
                {
                    std::cout << "fn_decl: ";
                    fn_decl fn = arg;
                    std::cout << fn.sig_type << " " << fn.name << "( ";
                    for ( size_t i = 0; i < fn.params.size(); ++i )
                    {
                        auto& p = fn.params[ i ];
                        std::cout << p.type << " " << p.name << ( i == fn.params.size() - 1 ?  "" : ", " );
                    }
                    
                    std::cout << " )\n";
                    print_stmt( fn.body, 1 );
                }
                else if constexpr ( std::is_same_v< T, struct_decl > )
                    std::cout << "struct_decl\n";
                else if constexpr ( std::is_same_v< T, enum_decl > )
                    std::cout << "enum_decl\n";
                else
                    static_assert( false, "non-exhaustive visitor!" );
            }, decl );
        }
    }
}
