#pragma once

#include <variant>
#include <vector>

#include "token.hpp"
#include "../sema/types.hpp"

namespace dungeon::ast {

struct stmt;
struct expr;
struct toplevel;

using expr_ptr = std::unique_ptr< expr >;
using stmt_ptr = std::unique_ptr< stmt >;
using toplevel_ptr = std::unique_ptr< toplevel >;

struct num_lit_data { uint64_t value; };
struct bool_lit_data { bool value; };
struct identifier_data { std::string_view id; };
struct unary_data { op_kind op; expr_ptr lhs; };
struct binary_data { op_kind op; expr_ptr lhs; expr_ptr rhs; };
struct relational_data{ op_kind op; expr_ptr lhs; expr_ptr rhs; };
struct assign_data{ identifier_data id; expr_ptr val; };
struct call_data{ expr_ptr callee; std::vector< expr_ptr > args; };

struct expr
{
    location src_loc;
    using data_t = std::variant< num_lit_data,
                        bool_lit_data,
                        identifier_data,
                        unary_data,
                        binary_data,
                        relational_data,
                        assign_data,
                        call_data >;
    data_t data;
};

struct ret_data{ expr_ptr val; };
struct if_data{ expr_ptr cond; stmt_ptr then_body; stmt_ptr else_body;};
struct for_data{ stmt_ptr init; expr_ptr cond; expr_ptr update; };
struct while_data{ expr_ptr cond; stmt_ptr body; };
struct do_while_data{ expr_ptr cond; stmt_ptr body; };
struct cont_data{};
struct brk_data{};
struct block_data{ std::vector< stmt_ptr > stmts; };

struct var_decl
{
    enum mod_t { mut, imut } modifier;
    std::string_view name;
};

struct expr_stmt_data{ expr_ptr expr; };

struct stmt
{
    location loc;
    using data_t = std::variant< ret_data,
                    if_data,
                    for_data,
                    while_data,
                    do_while_data,
                    cont_data,
                    brk_data,
                    block_data,
                    var_decl,
                    expr_stmt_data >;
    data_t data;
};

struct enum_decl
{
    std::string_view id;
    std::vector< std::string_view > fields;
};

struct struct_decl
{
    std::string_view name;
    std::vector< std::string_view > fields;
};

struct fn_decl
{
    std::string_view name;
    std::vector< stmt_ptr > body;
    std::vector< std::string_view > params;
};

struct toplevel
{
    location loc;
    using data = std::variant< fn_decl, enum_decl, struct_decl >;
};

struct module
{
    std::vector< toplevel > toplevel_items;
};

struct program
{
    std::vector< module > modules;
};

} // namespace dungeon
