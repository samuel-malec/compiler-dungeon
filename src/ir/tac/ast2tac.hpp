#include <map>
#include <string>
#include <vector>

#include "../../frontend/ast.hpp"
#include "tac.hpp"

namespace dungeon::tac
{

// TODO: we should use the atom table we have built in semantic analysis and use it here to boost our performance a little bit
struct scope_manager
{
    using scope = std::map< std::string, tmp >;
    std::vector< scope > scopes;

    void push_scope() { scopes.push_back( {} ); }
    void pop_scope() { scopes.pop_back(); }
    
    // The optional is not really necessaray here, because the program passed semantical analysis
    std::optional< tmp > get_var( std::string var )
    {
        for ( int i = scopes.size() - 1; i >= 0; --i )
        {
            auto it = scopes[ i ].find( var );
            if ( it != scopes[ i ].end() )
                return it->second;
        }
        return {};
    }

    void add_var( std::string var, tmp t )
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
            return top::sub;
        if ( op == ast::op_kind::MUL )
            return top::mul;
        if ( op == ast::op_kind::DIV )
            return top::div;
        if ( op == ast::op_kind::MOD )
            return top::mod;
        if ( op == ast::op_kind::SHL )
            return top::shl;
        if ( op == ast::op_kind::SHR )
            return top::shr;
        if ( op == ast::op_kind::EQ )
            return top::eq;
        if ( op == ast::op_kind::NEQ )
            return top::neq;
        if ( op == ast::op_kind::LT )
            return top::lt;
        if ( op == ast::op_kind::LEQ )
           return top::leq;
        if ( op == ast::op_kind::GT )
            return top::gt;
        if ( op == ast::op_kind::GEQ )
           return top::geq;
        if ( op == ast::op_kind::AND )
           return top::land; 

        return top::lor;
    }

    void add_instr( instr::data_type dt )
    {
        instr i{ .data = dt };
        ins.push_back( i );
    }

    void add_label( std::string name )
    {
        label_data ld{ .name = name };
        instr i{ .data = ld };
        ins.push_back( i );
    }

    tmp lower_expr( ast::expr& e )
    {
        using namespace ast;
        switch ( e.cat )
        {
            case expr::num_lit:
            {
                copy_data cp{ .arg1 = std::get< uint64_t >( e.val ), .target = tmp{ get_id() } };
                add_instr( cp );
                return tmp{ tmp_id - 1 };
            }

            case expr::bool_lit:
            {
                copy_data cp{ .arg1 = std::get< bool >( e.val ), .target = tmp{ get_id() } };
                add_instr( cp );
                return tmp{ tmp_id - 1 };
            }

            case expr::identifier:
            {
                return sm.get_var( std::string( e.id ) ).value();
            }

            case expr::unary:
            {
                tmp tmp1 = lower_expr( e[ 0 ] );
                unary_data ud{ .op = get_un_op( e.op ), .arg1 = tmp{ tmp1 }, .target = tmp{ get_id() } };
                add_instr( ud );
                return tmp{ tmp_id - 1 };
            }

            case expr::binary:
            case expr::relational:
            {
                tmp tmp1 = lower_expr( e[ 0 ] );
                tmp tmp2 = lower_expr( e[ 1 ] );
                binary_data bd{ .op = get_bin_op( e.op ), .arg1 = tmp1, .arg2 = tmp2, .target = tmp{ get_id() } };
                add_instr( bd );
                return tmp{ tmp_id - 1 };
            }

            case expr::assign:
            {
                // We need to check whether the lhs of the assignment is a variable, 
                // which would result in the update of the tmp_id of that variable   
                // Currently, this is easy, since the only lvalue we can currently create is a variable,
                // but things will get more complicated once we add pointers to our language...
                auto lhs = e[ 0 ];
                if ( lhs.id.empty() )
                {
                    // FIXME: Once our language contains other ways to create lvalues we will have to take this case into account...
                    assert( false );
                    return tmp{ -1 }; 
                }

                tmp src_tmp = lower_expr( e[ 1 ] );
                tmp dst_tmp = sm.get_var( std::string( lhs.id ) ).value();
                copy_data cd{ .arg1 = src_tmp, .target = dst_tmp };
                add_instr( cd );
                return dst_tmp;
            }

            case expr::call:
            {
                auto callee = e[ 0 ];
                std::vector< tmp > arg_tmps{};
                
                for ( int i = 1; i < e.subs.size(); ++ i )
                    arg_tmps.push_back( lower_expr( e[ i ] ) );
                
                for ( auto tmp : arg_tmps )
                {
                    param_data pd{ .arg = tmp };
                    add_instr( pd );
                }
                
                call_data cd{ .callee = std::string( callee.id ), .args = static_cast< int >( arg_tmps.size() ), .target = tmp{ get_id() } };
                add_instr( cd );
                return tmp{ tmp_id - 1 };
            }

            default:
                break;
        }

        return tmp{ -1 };
    }

    void lower_stmt( ast::stmt& s )
    {
        using namespace ast;
        switch ( s.cat )
        {
            case stmt::ret:
            {
                if ( !s.e )
                {
                    ret_data rd{ .arg = std::nullopt };
                    add_instr( rd );
                    return;
                }
                
                tmp tmp1 = lower_expr( s.e.value() );
                ret_data rd{ .arg = tmp1 };
                add_instr( rd );
                break;
            }

            case stmt::if_stmt:
            {
                tmp cond_tmp = lower_expr( s.e.value() );
                std::string body_label = gen_label( "if_body" );
                std::string endif_label = gen_label( "endif" );
                branch_if_data bid{ .arg1 = cond_tmp, .label = body_label };
                add_instr( bid );

                jump_data jd{ .label = endif_label };
                add_instr( jd );

                add_label( body_label );
                lower_stmt( s[ 0 ] );

                add_label( endif_label );
                if ( s.subs.size() > 1 )
                    lower_stmt( s[ 1 ] );

                break;
            }

            case stmt::for_stmt:
            {
                sm.push_scope();
                std::string init_label = gen_label( "for_init" );
                std::string cond_label = gen_label( "for_cond" );
                std::string out_label = gen_label( "for_out" );
                std::string body_label = gen_label( "for_body" );

                add_label( init_label );
                lower_stmt( s[ 0 ] );

                add_label( cond_label );
                lower_stmt( s[ 1 ] );
                // we are using the fact that we have just lowered the result of the expression
                branch_if_data bid{ .arg1 = tmp{ .id = tmp_id - 1 }, .label = body_label };
                add_instr( bid );

                jump_data out_jump{ .label = out_label };
                add_instr( out_jump );
                
                add_label( body_label );
                for ( int i = 3; i < s.subs.size(); ++i )
                    lower_stmt( s[ i ] );

                lower_stmt( s[ 2 ] );
                jump_data cond_jump{ .label = cond_label };
                add_instr( cond_jump );
                
                add_label( out_label );
                sm.pop_scope();
                break;
            }

            case stmt::while_stmt:
            {
                std::string head_label = gen_label( "while_head" );
                std::string out_label = gen_label( "while_out" );
                std::string body_label = gen_label( "while_body" );
                
                add_label( head_label );
                
                auto tmp1 = lower_expr( s.e.value() );
                branch_if_data bid{ .arg1 = tmp1, .label = body_label };
                add_instr( bid );

                jump_data out_jump{ .label = out_label };
                add_instr( out_jump );

                add_label( body_label );
                sm.push_scope();
            
                for ( auto& sub : s.subs )
                    lower_stmt( sub );

                sm.pop_scope();

                jump_data jd{ .label = head_label };
                add_label( out_label );
                break;
            }

            case stmt::do_while_stmt:
            {
                std::string head_label = gen_label( "do_head" );
                sm.push_scope();
                add_label( head_label );
                for ( auto& sub : s.subs )
                    lower_stmt( sub );

                sm.pop_scope();
                auto tmp1 = lower_expr( s.e.value() );
                branch_if_data bid{ .arg1 = tmp1, .label = head_label };
                add_instr( bid );
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
                sm.push_scope();
                for ( auto& s : s.subs )
                    lower_stmt( s );
                sm.pop_scope();
                break;
            }

            case stmt::var_dclr:
            {
                if ( s.vdecl.e.has_value() )
                {
                    tmp tmp1 = lower_expr( s.vdecl.e.value() );
                    sm.add_var( std::string( s.vdecl.name ), tmp1 );
                    return;
                }
                sm.add_var( std::string( s.vdecl.name ), tmp{ .id = get_id() } );
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
    b.sm.push_scope();
    for ( int i = 0; i < fn.params.size(); ++i )
    {
        tmp target = tmp{ .id = b.get_id() };
        b.sm.add_var( std::string( fn.params[ i ].name ), target );
        get_param_data gpd{ .idx = i, .target = target };
        b.add_instr( gpd );
    }

    for ( auto& stmt : fn.body )
        b.lower_stmt( stmt );
    b.sm.pop_scope();

    tac_fn.body = std::move( b.ins );
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
