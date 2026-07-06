#include <map>
#include <string>
#include <vector>

#include "../../frontend/ast.hpp"
#include "tac.hpp"

namespace dungeon::tac
{

struct builder
{
    int tmp_id = 0;
    int lab_id = 0;
    std::vector< instr > ins;
    std::map< std::string, int > var_to_id;

    std::string gen_label( std::string name = "" )
    {
        return "lab_" + name + std::to_string( lab_id++ );
    }
    
    int get_id() 
    { 
        return tmp_id++;
    }

    un_op get_un_op( ast::op_kind op )
    {
        if ( op == ast::op_kind::NOT )
            return tac::un_op::lnot;
        if ( op == ast::op_kind::ADD )
            return tac::un_op::plus;
        return tac::un_op::minus;    
    }

    bin_op get_bin_op( ast::op_kind op )
    {
        using top = tac::bin_op;

        if ( op == ast::op_kind::ADD )
            return top::add;
        if ( op == ast::op_kind::SUB )
            return top::add;
        if ( ast::op_kind::MUL )
            return top::mul;
        if ( ast::op_kind::DIV )
            return top::div;
        if ( ast::op_kind::MOD )
            return top::mod;
        if ( ast::op_kind::SHL )
            return top::shl;
        if ( ast::op_kind::SHR )
            return top::shr;
        if ( ast::op_kind::EQ )
            return top::eq;
        if ( ast::op_kind::NEQ )
            return top::neq;
        if ( ast::op_kind::LT )
            return top::lt;
        if ( ast::op_kind::LEQ )
           return top::leq;
        if ( ast::op_kind::GT )
            return top::gt;
        if ( ast::op_kind::GEQ )
           return top::geq;
        if ( ast::op_kind::AND )
           return top::land; 

        return top::lor;
    }

    
    void add_instr( instr::data_type dt )
    {
        instr i{ .data = dt };
        ins.push_back( i );
    }

    int lower_expr( ast::expr& e )
    {
        using namespace ast;
        switch ( e.cat )
        {
            case expr::num_lit:
            {
                copy_data cp{ .arg1 = std::get< uint64_t >( e.val ), .target = get_id() };
                add_instr( cp );
                return tmp_id - 1;
            }

            case expr::bool_lit:
            {
                copy_data cp{ .arg1 = std::get< bool >( e.val ), .target = get_id() };
                add_instr( cp );
                return tmp_id - 1;
            }

            case expr::identifier:
            {
                // copy_data cp{ .arg1 = } 
                break;
            }

            case expr::unary:
            {
                lower_expr( e[ 0 ] );
                unary_data ud{ .op = get_un_op( e.op ), .arg1 = tmp_id, .target = get_id() };
                return tmp_id - 1;
            }

            case expr::binary:
            case expr::relational:
            {
                int tmp1 = lower_expr( e[ 0 ] );
                int tmp2 = lower_expr( e[ 1 ] );
                binary_data bd{ .op = get_bin_op( e.op ), .arg1 = tmp1, .arg2 = tmp2, .target = get_id() };
                return tmp_id - 1;
            }

            case expr::assign:
            {
                break;
            }

            case expr::call:
            {
                break;
            }

            default:
                break;
        }

        return tmp_id;
    }

    void lower_stmt( ast::stmt& s )
    {
        using namespace ast;
        switch ( s.cat )
            case stmt::ret:
            {
                break;
            }
            case stmt::if_stmt:
            {
                break;
            }
            case stmt::for_stmt:
            {
                break;
            }
            case stmt::while_stmt:
            {
                break;
            }
            case stmt::do_while_stmt:
            {
                break;
            }
            case stmt::cont:
            {
                break;
            }
            case stmt::brk:
            {
                break;
            }
            case stmt::block:
            {
                break;
            }
            case stmt::var_dclr:
            {
                break;
            }
            case stmt::expr_stmt:
            {
                lower_expr( s.e.value() );
                break;
            }
        }
    }       
};

function lower_to_tac( ast::fn_decl& fn )
{
    function tac_fn{};
    tac_fn.name = fn.name;
    tac_fn.sig = fn.sig;
    
    builder b{};
    for ( auto& stmt : fn.body )
        b.lower_stmt( stmt );

    tac_fn.body = b.ins;
    return tac_fn;
}

program lower_to_tac( ast::program& ast )
{
    program prog{};

    for ( auto& decl : ast.decls )
    {
        if ( std::holds_alternative< ast::fn_decl >( decl ) )
        {
            ast::fn_decl fn = std::get< ast::fn_decl >( decl );
            function tac_fn = lower_to_tac( fn );                
            prog.functions.push_back( tac_fn );
        }
    }

    return prog;
}

}
