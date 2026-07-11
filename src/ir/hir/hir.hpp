#pragma once

#include <cstdint>

#include "../../frontend/types.hpp"

namespace dungeon::hir
{

using var_id = std::uint32_t;
using fn_id = std::uint32_t;

struct expr
{
    enum kind_t
    {
        int_lit,
        bool_lit,
        var_ref,
        unary,
        binary,
        assign,
        call,
        cast
    } kind;

    type typ;
    struct var_ref_data { var_id id; };
    struct unary_data   { op_kind op; expr* sub; };
    struct binary_data  { op_kind op; expr* left; expr* right; };
    struct assign_data  { var_id target; expr* value; };
    struct call_data    { fn_id target; std::vector<expr> args; };
    
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
    enum class kind_t 
    {
        expr_stmt,
        block,
        let_stmt,
        if_stmt,
        loop_stmt,
        brk,
        cont,
        ret
    } kind;

    struct block_data { std::vector<stmt> stmts; };
    struct let_data   { var_id target; expr value; };
    struct if_data    { expr cond; stmt* then_branch; stmt* else_branch; };
    struct loop_data  { stmt* body; };
    struct ret_data   { expr* value; };

    std::variant<
        expr,
        block_data,
        let_data,
        if_data,
        loop_data,
        std::monostate,
        ret_data
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
    // optionally we can add global vars
}

}