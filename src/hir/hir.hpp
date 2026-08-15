#pragma once

#include <cassert>
#include <variant>
#include <memory>

#include "../sema/semantic.hpp"
#include "../sema/types.hpp"

namespace dungeon::hir
{
    struct var_id { uint32_t idx; };
    struct expr_id { uint32_t idx; };
    struct stmt_id { uint32_t idx; };

    struct expr
    {
        type ty;
        struct var_data     { var_id id; };
        struct unary_data   { op_kind op; expr_id lhs; };
        struct binary_data  { op_kind op; expr_id left; expr_id right; };
        struct assign_data  { var_id target; expr_id value; };
        struct call_data    { fn_id target; std::vector< expr_id > args; };

        std::variant<
            uint64_t,
            bool,
            var_data,
            unary_data,
            binary_data,
            assign_data,
            call_data
        > data;
    };

    struct stmt
    {
        struct expr_data  { expr_id e; };
        struct block_data { std::vector< stmt_id > stmts; };
        struct let_data   { var_id target; expr_id value; };
        struct if_data    { expr cond; stmt_id then_branch; stmt_id else_branch; };
        struct loop_data  { stmt_id body; };
        struct ret_data   { expr_id value; };
        struct brk        {};
        struct cont       {};

        std::variant<
            expr_data,
            block_data,
            let_data,
            if_data,
            loop_data,
            std::monostate,
            ret_data,
            brk,
            cont
        > data;
    };

    struct function
    {
        stmt_id root;
        std::vector< binding_id > params;
        std::vector< expr > exprs;
        std::vector< stmt > stmts;

        expr& get_expr( int idx ) { assert( idx >= 0 && idx < exprs.size() ); return exprs[ idx ]; }
        stmt& get_stmt( int idx ) { assert( idx >= 0 && idx < stmts.size() ); return stmts[ idx ]; }

    };

    struct module
    {
        std::vector< function> functions;
    };

    struct program
    {
        std::vector< module > modules;
    };

}

