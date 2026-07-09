#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include "../frontend/ast.hpp"
#include "../ir/tac/tac.hpp"

// TODO: Right now this is a utility printer for debugging purposes, later
// we should add a more sophisticated logging system, which would allow us to
// log information about compiler phases, such as duration of the phase,
// and specific metadata.

namespace dungeon::print
{

struct pretty_printer
{
    using expr = ast::expr;
    using stmt = ast::stmt;
    using toplevel = ast::toplevel;
    using var_decl = ast::var_decl;
    using fn_decl = ast::fn_decl;
    using enum_decl = ast::enum_decl;
    using struct_decl = ast::struct_decl; 

    std::string indent( int depth )
    {
        return std::string( depth * 2, ' ' );
    }

    void pad( int depth )
    {
        std::cout << indent( depth );
    }

    inline void print_expr( expr& e, int depth )
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

    inline void print_stmt( stmt& s, int depth )
    {
        pad( depth );
        switch ( s.cat )
        {
            case stmt::ret:
                std::cout << "[ ret ]";
                if  ( s.e.has_value() )
                {
                    std::cout << "\n";
                    print_expr( s.e.value(), depth + 1 );
                }
                else
                    std::cout << " {}\n";
                return;
            
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
                std::cout << "[ var ] " << s.vdecl.typ << " " << s.vdecl.name << '\n';
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

    inline void print_ast( ast::program& ast )
    {
        for ( auto& decl : ast.toplevel_items )
        {
            std::visit( [ this ]( auto&& arg )
            {
                using T = std::decay_t< decltype( arg ) >;
                if constexpr ( std::is_same_v< T, fn_decl > )
                {
                    std::cout << "fn_decl: ";
                    fn_decl fn = arg;
                    std::cout << fn.sig.ret_type << " " << fn.name << "( ";
                    for ( size_t i = 0; i < fn.params.size(); ++i )
                    {
                        auto& p = fn.params[ i ];
                        std::cout << p.typ << " " << p.name << ( i == fn.params.size() - 1 ?  "" : ", " );
                    }
                    
                    std::cout << " )\n";
                    for ( auto& s : fn.body )
                        print_stmt( s, 1 );
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

    std::string tac_tmp_to_string( const tac::tmp& t )
    {
        return "t" + std::to_string( t.id );
    }

    std::string tac_arg_to_string( const tac::argument& arg )
    {
        return std::visit( [ this ]( auto&& value ) -> std::string
        {
            using T = std::decay_t< decltype( value ) >;
            if constexpr ( std::is_same_v< T, tac::tmp > )
                return tac_tmp_to_string( value );
            else
            {
                std::ostringstream out;
                if ( std::holds_alternative< uint64_t >( value ) )
                    out << std::get< uint64_t >( value );
                if ( std::holds_alternative< bool >( value ) )
                    out << ( std::get< bool >( value ) ? "true" : "false" );
                return out.str();
            }
        }, arg );
    }

    std::string tac_un_op_to_string( tac::un_op op )
    {
        switch ( op )
        {
            case tac::un_op::plus:  return "+";
            case tac::un_op::minus: return "-";
            case tac::un_op::lnot:  return "!";
        }
        return "?";
    }

    std::string tac_bin_op_to_string( tac::bin_op op )
    {
        switch ( op )
        {
            case tac::bin_op::add:  return "+";
            case tac::bin_op::sub:  return "-";
            case tac::bin_op::mul:  return "*";
            case tac::bin_op::div:  return "/";
            case tac::bin_op::mod:  return "%";
            case tac::bin_op::shl:  return "<<";
            case tac::bin_op::shr:  return ">>";
            case tac::bin_op::eq:   return "==";
            case tac::bin_op::neq:  return "!=";
            case tac::bin_op::lt:   return "<";
            case tac::bin_op::leq:  return "<=";
            case tac::bin_op::gt:   return ">";
            case tac::bin_op::geq:  return ">=";
            case tac::bin_op::land: return "&&";
            case tac::bin_op::lor:  return "||";
        }
        return "?";
    }

    std::string tac_instr_symbolic( const tac::instr& i )
    {
        return std::visit( [ this ]( auto&& value ) -> std::string
        {
            using T = std::decay_t< decltype( value ) >;
            std::ostringstream out;

            if constexpr ( std::is_same_v< T, tac::unary_data > )
            {
                out << tac_tmp_to_string( value.target ) << " = "
                    << tac_un_op_to_string( value.op ) << " "
                    << tac_arg_to_string( value.arg1 );
            }
            else if constexpr ( std::is_same_v< T, tac::binary_data > )
            {
                out << tac_tmp_to_string( value.target ) << " = "
                    << tac_arg_to_string( value.arg1 ) << " "
                    << tac_bin_op_to_string( value.op ) << " "
                    << tac_arg_to_string( value.arg2 );
            }
            else if constexpr ( std::is_same_v< T, tac::copy_data > )
            {
                out << tac_tmp_to_string( value.target ) << " = "
                    << tac_arg_to_string( value.arg1 );
            }
            else if constexpr ( std::is_same_v< T, tac::jump_data > )
            {
                out << "goto ";
                out << value.label;
            }
            else if constexpr ( std::is_same_v< T, tac::branch_if_data > )
            {
                out << "if " << tac_arg_to_string( value.arg1 )
                    << " goto " << value.label;
            }
            else if constexpr ( std::is_same_v< T, tac::param_data > )
            {
                out << "param ";
                out << tac_arg_to_string( value.arg );
            }
            else if constexpr ( std::is_same_v< T, tac::get_param_data> )
            {
                out << tac_tmp_to_string( value.target ) << " = get_param "
                << value.idx;
            }
            else if constexpr ( std::is_same_v< T, tac::call_data > )
            {
                out << tac_tmp_to_string( value.target ) << " = call "
                    << value.callee << "(" << value.args << " args)";
            }
            else if constexpr ( std::is_same_v< T, tac::label_data > )
            {
                out << value.name << ":";
            }
            else if constexpr ( std::is_same_v< T, tac::ret_data > )
            {
                out << "ret";
                if ( value.arg.has_value() )
                    out << " " << tac_arg_to_string( value.arg.value() );
            }

            return out.str();
        }, i.data );
    }

    inline void print_tac_inst( tac::instr& i )
    {
        std::holds_alternative< tac::label_data>( i.data ) ? pad( 2 ) : pad( 4 );
        std::cout << tac_instr_symbolic( i ) << '\n';
    }

    inline void print_tac( tac::program& tac )
    {
        std::cout << "\n";
        std::cout << "Printing three address code\n";
        std::cout << "__________________________________\n";
        for ( auto& fn : tac.functions )
        {
            std::cout << fn.name << " :: ";
            std::cout << fn.sig;
            for ( auto& i : fn.body )
                print_tac_inst( i );
        }
    }
};

}
