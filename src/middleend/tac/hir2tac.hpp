#pragma once

#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../hir/hir.hpp"
#include "tac.hpp"

namespace dungeon::tac
{

struct scope_manager
{
    struct loop_context
    {
        label_id continue_target;
        label_id break_target;
    };

    using symbol_id = uint32_t;
    using value_id = uint32_t;
    using scope = std::map< symbol_id, value_id >;

    std::vector< scope > scopes;
    std::vector< loop_context > loop_stack;

    void push_scope() { scopes.push_back( {} ); }
    void pop_scope() { scopes.pop_back(); }
    
    value lookup( uint32_t symbol_id )
    {
        for ( int i = scopes.size() - 1; i >= 0; --i )
        {
            auto it = scopes[ i ].find( symbol_id );
            if ( it != scopes[ i ].end() )
                return value{ .id = it->second };
        }
        assert( false && "unknown variable" );
        return {};
    }

    value bind( uint32_t symbol_id, value fresh )
    {
        scopes.back()[ symbol_id ] = fresh.id;
        return fresh;
    }
};

struct builder
{
    uint32_t next_value = 0;
    uint32_t next_label = 1;

    std::vector< instr > ins;
    scope_manager sm{};

    value create_value()
    {
        return value{ .id = next_value++ };
    }

    label_id create_label()
    {
        return next_label++;
    }

    void emit( instr::data_type dt )
    {
        ins.push_back( instr{ .data = std::move( dt ) } );
    }

    void add_label( label_id id )
    {
        label_data ld{ .id = id };
        instr i{ .data = ld };
        ins.push_back( i );
    }

    operand lower_expr( hir::expr& e )
    {
        switch ( e.kind )
        {
            case hir::expr::int_lit:
            {
                return constant{ std::get< uint64_t >( e.data ) };
            }
            case hir::expr::bool_lit:
            {
                return constant{ std::get< bool >( e.data ) };
            }
            case hir::expr::var_ref:
            {
                auto data = std::get< hir::expr::var_ref_data >( e.data );
                return sm.lookup( data.id );
            }
            case hir::expr::unary:
            {
                auto data = std::get< hir::expr::unary_data >( e.data );
                operand arg1 = lower_expr( *data.sub );
                auto target = create_value();

                unary_data ud{ .op = data.op, .arg1 = arg1, .target = target };
                emit( ud );
                return target;
            }
            case hir::expr::binary:
            {
                auto data = std::get< hir::expr::binary_data >( e.data );
                auto lhs = lower_expr( *data.left );
                auto rhs = lower_expr( *data.right );
                auto target = create_value();

                binary_data bd{ .op = data.op, .arg1 = lhs, .arg2 = rhs, .target = target };
                emit( bd );
                return target;
            }
            case hir::expr::assign:
            {
                auto data = std::get< hir::expr::assign_data >( e.data );
                operand arg1 = lower_expr( *data.value );
                value target = sm.lookup( data.target );

                copy_data cd{ .arg1 = arg1, .target = target };
                emit( cd );
                return target;
            }
            case hir::expr::call:
            {
                auto data = std::get< hir::expr::call_data >( e.data );
                auto callee = data.target;
                std::vector< operand > call_param_locs{};
                for ( int i = 0; i < data.args.size(); ++ i )
                    call_param_locs.push_back( lower_expr( *data.args[ i ] ) );
                
                for ( auto tmp : call_param_locs )
                {
                    param_data pd{ .arg = tmp };
                    emit( pd );
                }
                
                auto target = create_value();
                call_data cd{ .callee = callee, .args = static_cast< int >( call_param_locs.size() ), .target = target };
                emit( cd );
                return target;
            }
            case hir::expr::cast:
            {
                break;
            }   
            default:
                break;
        }

        assert( false );
    }

    void lower_stmt( hir::stmt& s )
    {
        switch ( s.kind )
        {
            case hir::stmt::kind_t::expr_stmt:
            {
                lower_expr( std::get< hir::expr >( s.data ) );
                break;
            }
            case hir::stmt::kind_t::block:
            {
                auto data = std::get< hir::stmt::block_data >( s.data );
                sm.push_scope();
                for ( auto& sub_stmt : data.stmts )
                    lower_stmt( sub_stmt );
                sm.pop_scope();
                break;
            }
            case hir::stmt::kind_t::let_stmt:
            {
                auto data = std::get< hir::stmt::let_data >( s.data );
                value target = sm.bind( data.target, create_value() ); 

                if ( data.value )
                {
                    operand arg1 = lower_expr( *data.value );
                    copy_data cd{ .arg1 = arg1, .target = target };
                    emit( cd );
                }
                else if ( data.typ.as_primitive() == BOOL )
                {
                    copy_data cd{ .arg1 = constant{ false }, .target = target };
                    emit( cd );
                }
                else if ( data.typ.as_primitive() == INT )
                {
                    copy_data cd{ .arg1 = constant{ uint64_t{ 0 } }, .target = target };
                    emit( cd );
                }
                else
                    assert( false );
                break;
            }
            case hir::stmt::kind_t::if_stmt:
            {
                auto data = std::get< hir::stmt::if_data >( s.data );
                operand cond_tmp = lower_expr( data.cond );
                label_id then_label = create_label();
                label_id else_label = create_label();
                label_id end_label = create_label();
               
                branch_data jd{ .arg1 = cond_tmp, .true_lab = then_label, .false_lab = data.else_branch ? else_label : end_label };
                emit( jd );

                add_label( then_label );
                lower_stmt( *data.then_branch );
                emit( jump_data{ .label = end_label } );

                if ( data.else_branch )
                {
                    add_label( else_label );
                    lower_stmt( *data.else_branch );
                    emit( jump_data{ .label = end_label } );
                }

                add_label( end_label );
                break;
            }
            case hir::stmt::kind_t::loop_stmt:
            {
                auto data = std::get< hir::stmt::loop_data >( s.data );
                auto head_lab = create_label();
                auto exit_lab = create_label();
                sm.loop_stack.push_back( { .continue_target = head_lab, .break_target = exit_lab } );
                
                add_label( head_lab );
                if ( data.body )
                    lower_stmt( *data.body );
                
                emit( jump_data{ .label = head_lab } );
                add_label( exit_lab );

                sm.loop_stack.pop_back();
                break;
            }
            case hir::stmt::kind_t::brk:
            {
                auto curr = sm.loop_stack.back();
                emit( jump_data{ .label = curr.break_target } );
                break;
            }
            case hir::stmt::kind_t::cont:
            { 
                auto curr = sm.loop_stack.back();
                emit( jump_data{ .label = curr.continue_target } );
                break;
            }
            case hir::stmt::kind_t::ret:
            {
                auto data = std::get< hir::stmt::ret_data >( s.data );
                if ( data.value )
                {
                    operand arg1 = lower_expr( *data.value );
                    ret_data rd{ .arg = arg1 };
                    emit( rd );
                    break;
                }
                ret_data rd{ .arg = std::nullopt };
                emit( rd );
                break;
            }
        }
    }     
};

tac::function lower_to_tac( hir::fn_def& fn )
{
    tac::function tac_fn{};
    tac_fn.name = fn.name;
    tac_fn.sig = fn.sig;
    
    builder b{};
    b.sm.push_scope();
    for ( int i = 0; i < fn.params.size(); ++i )
    {
        value target = b.sm.bind( fn.params[ i ], b.create_value() );
        get_param_data gpd{ .idx = i, .target = target };
        b.emit( gpd );
    }

    b.lower_stmt( fn.body );
    b.sm.pop_scope();

    tac_fn.body = std::move( b.ins );
    return tac_fn;
}

program lower_to_tac( hir::program& hir )
{
    program prog{};
    for ( hir::fn_def& fn_def : hir.functions )
    {
        const tac::function& tac_fn = lower_to_tac( fn_def );
        prog.functions.push_back( tac_fn );
    }

    return prog;
}

}
