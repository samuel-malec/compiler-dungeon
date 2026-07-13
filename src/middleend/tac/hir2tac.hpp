#include <map>
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

    using scope = std::map< uint32_t, tmp >;
    std::vector< scope > scopes;
    std::vector< loop_context > loop_stack;

    void push_scope() { scopes.push_back( {} ); }
    void pop_scope() { scopes.pop_back(); }
    
    // The optional is not really necessaray here, because the program passed semantical analysis
    std::optional< tmp > get_var( uint32_t var )
    {
        for ( int i = scopes.size() - 1; i >= 0; --i )
        {
            auto it = scopes[ i ].find( var );
            if ( it != scopes[ i ].end() )
                return it->second;
        }
        return {};
    }

    void add_var( uint32_t var, tmp t )
    {
        auto& last = scopes.back();
        last[ var ] = t;
    }
};

struct builder
{
    int tmp_id = 0;
    int lab_id = 0;
    std::vector< instr > ins;
    scope_manager sm{};

    label_id gen_label() { return lab_id++; }
    
    int get_id() { return tmp_id++; }

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

    tmp lower_expr( hir::expr& e )
    {
        switch ( e.kind )
        {
            case hir::expr::int_lit:
            {
                copy_data cp{ .arg1 = std::get< uint64_t >( e.data ), .target = tmp{ get_id() } };
                add_instr( cp );
                return tmp{ tmp_id - 1 };
            }
            case hir::expr::bool_lit:
            {
                copy_data cp{ .arg1 = std::get< bool >( e.data ), .target = tmp{ get_id() } };
                add_instr( cp );
                return tmp{ tmp_id - 1 };
            }
            case hir::expr::var_ref:
            {
                auto data = std::get< hir::expr::var_ref_data >( e.data );
                return sm.get_var( data.id ).value();
            }
            case hir::expr::unary:
            {
                auto data = std::get< hir::expr::unary_data >( e.data );
                unary_data ud{ .op = data.op, .arg1 = lower_expr( *data.sub ), .target = tmp{ get_id() } };
                add_instr( ud );
                return tmp{ tmp_id - 1 };
            }
            case hir::expr::binary:
            {
                auto data = std::get< hir::expr::binary_data >( e.data );
                tmp tmp1 = lower_expr( *data.left );
                tmp tmp2 = lower_expr( *data.right );
                binary_data bd{ .op = data.op, .arg1 = tmp1, .arg2 = tmp2, .target = tmp{ get_id() } };
                add_instr( bd );
                return tmp{ tmp_id - 1 };
            }
            case hir::expr::assign:
            {
                auto data = std::get< hir::expr::assign_data >( e.data );
                tmp src_tmp = lower_expr( *data.value );
                tmp dst_tmp = sm.get_var( data.target ).value();
                copy_data cd{ .arg1 = src_tmp, .target = dst_tmp };
                add_instr( cd );
                return dst_tmp;
            }
            case hir::expr::call:
            {
                auto data = std::get< hir::expr::call_data >( e.data );
                auto callee = data.target;
                
                std::vector< tmp > arg_tmps{};
                for ( int i = 0; i < data.args.size(); ++ i )
                    arg_tmps.push_back( lower_expr( *data.args[ i ] ) );
                
                for ( auto tmp : arg_tmps )
                {
                    param_data pd{ .arg = tmp };
                    add_instr( pd );
                }
                
                call_data cd{ .callee = callee, .args = static_cast< int >( arg_tmps.size() ), .target = tmp{ get_id() } };
                add_instr( cd );
                return tmp{ tmp_id - 1 };
            }
            case hir::expr::cast:
            {
                break;
            }   
            default:
                break;
        }

        return tmp{ -1 };
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
                if ( data.value )
                {
                    tmp tmp1 = lower_expr( *data.value );
                    sm.add_var( data.target, tmp1 );
                }
                else
                    sm.add_var( data.target, tmp{ .id = get_id() } );
                break;
            }
            case hir::stmt::kind_t::if_stmt:
            {
                auto data = std::get< hir::stmt::if_data >( s.data );
                tmp cond_tmp = lower_expr( data.cond );
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
                    tmp tmp1 = lower_expr( *data.value );
                    ret_data rd{ .arg = tmp1 };
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
        tmp target = tmp{ .id = b.get_id() };
        b.sm.add_var( fn.params[ i ], target );
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
