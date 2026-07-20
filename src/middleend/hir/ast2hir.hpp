#pragma once

#include <iostream>

#include "../../frontend/ast.hpp"
#include "../../sema/types.hpp"
#include "hir.hpp"

namespace dungeon::hir
{

inline hir::expr make_not_expr( const hir::expr& sub_expr )
{
    hir::expr res;        
    res.kind = hir::expr::kind_t::unary;
    res.typ = sub_expr.typ;
    hir::expr::unary_data ud{ .op = NOT, .sub = std::make_shared< expr >( sub_expr ) };
    res.data = ud;
    return res;
}

inline hir::stmt create_break()
{
    hir::stmt res;        
    res.kind = hir::stmt::kind_t::brk;
    res.data = std::monostate{};
    return res;
}

inline hir::stmt make_cond_break( const hir::expr& cond_expr )
{
    hir::stmt res;
    res.kind = hir::stmt::kind_t::if_stmt;

    hir::stmt::if_data id{};
    id.cond = make_not_expr( cond_expr );
    id.then_branch = std::make_shared< hir::stmt >( create_break() );
    id.else_branch = nullptr;
    res.data = id;

    return res;
}

hir::expr lower_expr_to_hir( ast::expr& e, symtab& st )
{
    hir::expr res;
    res.typ = e.typ;

    switch ( e.cat )
    {
        case ast::expr::num_lit:
        {
            res.kind = hir::expr::kind_t::int_lit;
            res.data = std::get< uint64_t >( e.val );
            break;
        }
        case ast::expr::bool_lit:
        {
            res.kind = hir::expr::kind_t::bool_lit;
            res.data = std::get< bool >( e.val );
            break;
        }
        case ast::expr::identifier:
        {
            res.kind = hir::expr::kind_t::var_ref;
            hir::expr::var_ref_data vrd{ .id = st.get( e.id ).idx };
            res.data = vrd;
            break;
        }
        case ast::expr::unary:
        {
            res.kind = hir::expr::kind_t::unary;
            hir::expr::unary_data ud{ .op = e.op, .sub = std::make_shared< expr >( lower_expr_to_hir( e[ 0 ], st ) ) };
            res.data = ud;
            break;
        }
        // TODO: in the future, we should add short-circuit and, or to hir;;;
        // TODO: we can use only a subset of relational operators and convert the remaining ones to this subset
        case ast::expr::relational:
        case ast::expr::binary:
        {
            res.kind = hir::expr::kind_t::binary;
            hir::expr::binary_data bd{ .op = e.op,
                                       .left = std::make_shared< expr >( lower_expr_to_hir( e[ 0 ], st ) ),
                                       .right = std::make_shared< expr >( lower_expr_to_hir( e[ 1 ], st ) ) };
            res.data = bd;
            break;
        }
        case ast::expr::assign:
        {
            res.kind = hir::expr::kind_t::assign;
            hir::expr::assign_data ad{};
            ad.target = st.get( e[ 0 ].id ).idx;
            if ( e.op == EQ )
                ad.value = std::make_shared< expr >( lower_expr_to_hir( e[ 1 ], st ) );
            else
            {
                hir::expr helper{ .kind = hir::expr::kind_t::binary };
                helper.typ = e.typ;
                hir::expr::binary_data bd{ .op = op_from_compound_asn( e.op ), 
                                       .left = std::make_shared< expr >( lower_expr_to_hir( e[ 0 ], st ) ),
                                       .right = std::make_shared< expr >( lower_expr_to_hir( e[ 1 ], st ) ) };
                helper.data = bd;
                ad.value = std::make_shared< expr >( helper ); 
            };

            res.data = ad;
            break;
        }
        case ast::expr::call:
        {
            res.kind = hir::expr::kind_t::call;
            std::cout << "[ call ]\n";
            hir::expr::call_data cd{ .target = st.get( e.id ).idx };
            for ( int i = 1; i < e.subs.size(); ++ i )
                cd.args.push_back( std::make_shared< expr >( lower_expr_to_hir( e[ i ], st ) ) );
        }
        default:
            break;
    }

    return res;
}


hir::stmt lower_stmt_to_hir( ast::stmt& s, symtab& st )
{
    hir::stmt res;
    switch ( s.cat )
    {
        case ast::stmt::ret:
        {
            res.kind = hir::stmt::kind_t::ret;
            hir::stmt::ret_data rd{};

            if  ( s.e.has_value() )
                rd.value = std::make_shared< expr >( lower_expr_to_hir( s.e.value(), st ) );
            else
                rd.value = nullptr;

            res.data = rd;    
            break;
        }

        case ast::stmt::if_stmt:
        {
            res.kind = hir::stmt::kind_t::if_stmt;
            hir::stmt::if_data id{};
            id.cond = lower_expr_to_hir( s.e.value(), st );
            id.then_branch = std::make_shared< stmt > ( lower_stmt_to_hir( s[ 0 ], st ) );
            if ( s.subs.size() > 1 )
                id.else_branch = std::make_shared< stmt >( lower_stmt_to_hir( s[ 1 ], st ) );
            else
                id.else_branch = nullptr;
            
            res.data = id;
            break;
        }

        case ast::stmt::for_stmt:
        {
            // s[ 0 ] // init
            // s[ 1 ] // cond
            // s[ 2 ] // update
            // s[ 3 ] // body 
            // init; loop { if ( !cond ) break; body; update }
            res.kind = hir::stmt::kind_t::block;
            hir::stmt::block_data bd{};

            bd.stmts.push_back( lower_stmt_to_hir( s[ 0 ], st ) );

            hir::stmt inner_loop;
            inner_loop.kind = hir::stmt::kind_t::loop_stmt;
            hir::stmt::loop_data inner_loop_data;

            hir::stmt loop_block;
            loop_block.kind = hir::stmt::kind_t::block;
            hir::stmt::block_data loop_block_data;
            loop_block_data.stmts.push_back( make_cond_break( lower_expr_to_hir( s[ 1 ].e.value(), st ) ) );
            loop_block_data.stmts.push_back( lower_stmt_to_hir( s[ 3 ], st ) );
            loop_block_data.stmts.push_back( lower_stmt_to_hir( s[ 2 ], st ) );
            loop_block.data = loop_block_data;

            inner_loop_data.body = std::make_shared< hir::stmt >( loop_block );
            inner_loop.data = inner_loop_data;
            
            bd.stmts.push_back( inner_loop );
            res.data = bd;
            break;
        }

        case ast::stmt::while_stmt:
        {
            // loop { if (!cond) break; body }
            res.kind = hir::stmt::kind_t::loop_stmt;
            
            hir::stmt::block_data loop_block;
            if ( s.e.has_value() )
                loop_block.stmts.push_back( make_cond_break( lower_expr_to_hir( s.e.value(), st ) ) );
            
            loop_block.stmts.push_back( lower_stmt_to_hir( s[ 0 ], st ) );            
            hir::stmt body_wrapper;
            body_wrapper.kind = hir::stmt::kind_t::block;
            body_wrapper.data = loop_block;
            
            res.data = hir::stmt::loop_data{ .body = std::make_shared< hir::stmt >( body_wrapper ) };
            break;
        }

        case ast::stmt::do_while_stmt:
        {
            // loop { body; if (!cond) break; }
            res.kind = hir::stmt::kind_t::loop_stmt;
            
            hir::stmt::block_data loop_block;
            loop_block.stmts.push_back( lower_stmt_to_hir( s[ 0 ], st ) );
            
            if ( s.e.has_value() )
                loop_block.stmts.push_back( make_cond_break( lower_expr_to_hir( s.e.value(), st ) ) );
            
            hir::stmt body_wrapper;
            body_wrapper.kind = hir::stmt::kind_t::block;
            body_wrapper.data = loop_block;
            
            res.data = hir::stmt::loop_data{ .body = std::make_shared< hir::stmt >( body_wrapper ) };
            break;
        }
        case ast::stmt::cont:
        {
            res.kind = hir::stmt::kind_t::cont;
            res.data = std::monostate{};
            break;
        }
        
        case ast::stmt::brk:
        {
            res.kind = hir::stmt::kind_t::brk;
            res.data = std::monostate{};
            break;
        }

        case ast::stmt::block:
        {
            res.kind = hir::stmt::kind_t::block;
            hir::stmt::block_data bd{};
            for ( auto& sub : s.subs )
                bd.stmts.push_back( lower_stmt_to_hir( sub, st ) );
            res.data = bd;
            break;
        }

        case ast::stmt::var_dclr:
        {
            res.kind = hir::stmt::kind_t::let_stmt;
            hir::stmt::let_data ld{ .typ = s.vdecl.typ, .target = st.get( s.vdecl.name ).idx };
            if ( s.vdecl.e.has_value() )
                ld.value = std::make_shared< hir::expr >( lower_expr_to_hir( s.vdecl.e.value(), st ) );
            else
                ld.value = nullptr;
            res.data = ld;
            break;
        }
        case ast::stmt::expr_stmt:
        {
            res.kind = hir::stmt::kind_t::expr_stmt;
            res.data = lower_expr_to_hir( s.e.value(), st );
            break;
        }
        default:
            break;
    }

    return res;
}

hir::fn_def lower_fn_to_hir( ast::fn_decl& fun, symtab& st )
{
    hir::fn_def res{};
    res.name = st.get( fun.name ).idx;
    res.sig = fun.sig;
    
    for ( auto& p : fun.params )
        res.params.push_back( st.get( p.name ).idx );

    hir::stmt body{ .kind = stmt::kind_t::block };
    stmt::block_data bd{};
    for ( auto& s : fun.body )
        bd.stmts.push_back( lower_stmt_to_hir( s, st ) );    

    body.data = bd;
    res.body = body; 
    return res;
}

hir::program lower_ast_to_hir( ast::program& ast, symtab& st  )
{
    hir::program prog{};
    for ( auto& toplevel : ast.toplevel_items )
        if ( std::holds_alternative< ast::fn_decl >( toplevel ) )
        {
            hir::fn_def fd = lower_fn_to_hir( std::get< ast::fn_decl >( toplevel ), st );
            prog.functions.push_back( fd );
        }
    return prog;
}

}
