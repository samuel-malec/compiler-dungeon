#pragma once

#include <variant>
#include <cstdint>
#include <memory>

#include "../../sema/types.hpp"

// TODO: we could refactor this into a flat list of expressions and use ids to this list instead of shared_ptr...
namespace dungeon::hir
{

using var_id = std::uint32_t;
using fn_id = std::uint32_t;

struct expr;
struct stmt;
using expr_ptr = std::shared_ptr< expr >;
using stmt_ptr = std::shared_ptr< stmt >;

struct expr
{
    type typ;
    struct var_ref_data { var_id id; };
    struct unary_data   { op_kind op; expr_ptr sub; };
    struct binary_data  { op_kind op; expr_ptr left; expr_ptr right; };
    struct assign_data  { var_id target; expr_ptr value; };
    struct call_data    { fn_id target; std::vector< expr_ptr > args; };
    
    std::variant<
        uint64_t,
        bool,
        var_ref_data,
        unary_data,
        binary_data,
        assign_data,
        call_data
    > data;
};

struct stmt
{
    struct expr_data  { expr e; };
    struct block_data { std::vector< stmt > stmts; };
    struct let_data   { type typ; var_id target; expr_ptr value; };
    struct if_data    { expr cond; stmt_ptr then_branch; stmt_ptr else_branch; };
    struct loop_data  { stmt_ptr body; };
    struct ret_data   { expr_ptr value; };
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

struct fn_def
{
    fn_id name;
    function_type sig;
    std::vector< var_id > params;
    stmt body;
};

struct program
{
    std::vector< fn_def > functions;
    std::vector< stmt::let_data > globals;
};

}
