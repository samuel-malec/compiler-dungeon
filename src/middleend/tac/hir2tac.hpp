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

    using scope = std::map< uint32_t, var_id > ; // source var name -> var id in function
    var_id next_var_id = 0;
    std::vector< scope > scopes;
    std::vector< loop_context > loop_stack;

    void push_scope() { scopes.push_back( {} ); }
    void pop_scope() { scopes.pop_back(); }
    
    var get_var( uint32_t var_name )
    {
        for ( int i = scopes.size() - 1; i >= 0; --i )
        {
            auto it = scopes[ i ].find( var_name );
            if ( it != scopes[ i ].end() )
                return var{ .source_name_idx = var_name, .id = it->second };
        }
        return {};
    }

    var add_var( uint32_t source_name_idx )
    {
        var_id fresh = next_var_id++;
        scopes.back()[ source_name_idx ] = fresh;
        return var{ .source_name_idx = source_name_idx, .id = fresh };
    }
};

struct builder
{
    uint32_t tmp_id = 0;
    uint32_t lab_id = 0;

    std::vector< instr > ins;
    scope_manager sm{};

    label_id gen_label() { return lab_id++; }
    
    uint32_t get_tmp_id() { return tmp_id++; }

    void add_instr( instr::data_type dt )
    {
        instr i{ .data = dt };
        ins.push_back( i );
    }

    void add_label( label_id id )
    {
        label_data ld{ .id = id };
        instr i{ .data = ld };
        ins.push_back( i );
    }

    loc create_tmp() { return tmp{ .id = get_tmp_id() }; }

    argument lower_expr( hir::expr& e )
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
                return sm.get_var( data.id );
            }
            case hir::expr::unary:
            {
                auto data = std::get< hir::expr::unary_data >( e.data );
                argument arg1 = lower_expr( *data.sub );
                auto target = create_tmp();

                unary_data ud{ .op = data.op, .arg1 = lower_expr( *data.sub ), .target = target };
                add_instr( ud );
                return target;
            }
            case hir::expr::binary:
            {
                auto data = std::get< hir::expr::binary_data >( e.data );
                argument tmp1 = lower_expr( *data.left );
                argument tmp2 = lower_expr( *data.right );
                auto target = create_tmp();
            
                binary_data bd{ .op = data.op, .arg1 = tmp1, .arg2 = tmp2, .target = target };
                add_instr( bd );
                return target;
            }
            case hir::expr::assign:
            {
                auto data = std::get< hir::expr::assign_data >( e.data );
                argument arg1 = lower_expr( *data.value );
                loc tar = sm.get_var( data.target );

                copy_data cd{ .arg1 = arg1, .target = tar };
                add_instr( cd );
                return tar;
            }
            case hir::expr::call:
            {
                // FIXME: this seems kinda off but I am sleepy so maybe im missing something...
                auto data = std::get< hir::expr::call_data >( e.data );
                auto callee = data.target;
                std::vector< argument > call_param_locs{};
                for ( int i = 0; i < data.args.size(); ++ i )
                    call_param_locs.push_back( lower_expr( *data.args[ i ] ) );
                
                for ( auto tmp : call_param_locs )
                {
                    param_data pd{ .arg = tmp };
                    add_instr( pd );
                }
                
                auto target = create_tmp();
                call_data cd{ .callee = callee, .args = static_cast< int >( call_param_locs.size() ), .target = target };
                add_instr( cd );
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
                loc target = sm.add_var( data.target ); 

                if ( data.value )
                {
                    argument arg1 = lower_expr( *data.value );
                    copy_data cd{ .arg1 = arg1, .target = target };
                    add_instr( cd );
                }
                else if ( data.typ.as_primitive() == BOOL )
                {
                    copy_data cd{ .arg1 = constant{ false }, .target = target };
                    add_instr( cd );
                }
                else if ( data.typ.as_primitive() == INT )
                {
                    copy_data cd{ .arg1 = constant{ uint64_t{ 0 } }, .target = target };
                    add_instr( cd );
                }
                else
                    assert( false );
                break;
            }
            case hir::stmt::kind_t::if_stmt:
            {
                auto data = std::get< hir::stmt::if_data >( s.data );
                argument cond_tmp = lower_expr( data.cond );
                label_id then_label = gen_label();
                label_id else_label = gen_label();
                label_id end_label = gen_label();
               
                branch_data jd{ .arg1 = cond_tmp, .true_lab = then_label, .false_lab = data.else_branch ? else_label : end_label };
                add_instr( jd );

                add_label( then_label );
                lower_stmt( *data.then_branch );
                add_instr( jump_data{ .label = end_label } );

                if ( data.else_branch )
                {
                    add_label( else_label );
                    lower_stmt( *data.else_branch );
                    add_instr( jump_data{ .label = end_label } );
                }

                add_label( end_label );
                break;
            }
            case hir::stmt::kind_t::loop_stmt:
            {
                auto data = std::get< hir::stmt::loop_data >( s.data );
                auto head_lab = gen_label();
                auto exit_lab = gen_label();
                sm.loop_stack.push_back( { .continue_target = head_lab, .break_target = exit_lab } );
                
                add_label( head_lab );
                if ( data.body )
                    lower_stmt( *data.body );
                
                add_instr( jump_data{ .label = head_lab } );
                add_label( exit_lab );

                sm.loop_stack.pop_back();
                break;
            }
            case hir::stmt::kind_t::brk:
            {
                auto curr = sm.loop_stack.back();
                add_instr( jump_data{ .label = curr.break_target } );
                break;
            }
            case hir::stmt::kind_t::cont:
            { 
                auto curr = sm.loop_stack.back();
                add_instr( jump_data{ .label = curr.continue_target } );
                break;
            }
            case hir::stmt::kind_t::ret:
            {
                auto data = std::get< hir::stmt::ret_data >( s.data );
                if ( data.value )
                {
                    argument arg1 = lower_expr( *data.value );
                    ret_data rd{ .arg = arg1 };
                    add_instr( rd );
                    break;
                }
                else
                {
                    ret_data rd{ .arg = std::nullopt };
                    add_instr( rd );
                }
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
        loc target = b.sm.add_var( fn.params[ i ] );
        get_param_data gpd{ .idx = i, .target = target };
        b.add_instr( gpd );
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
