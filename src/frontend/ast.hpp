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

struct type_annotation
{
    std::string_view base_name;
};

struct ret_data{ expr_ptr val; };
struct if_data{ expr_ptr cond; stmt_ptr then_body; stmt_ptr else_body; };
struct for_data{ stmt_ptr init; expr_ptr cond; expr_ptr update; stmt_ptr body; };
struct while_data{ expr_ptr cond; stmt_ptr body; };
struct do_while_data{ expr_ptr cond; stmt_ptr body; };
struct cont_data{};
struct brk_data{};
struct block_data{ std::vector< stmt_ptr > stmts; };

struct var_decl
{
    enum mut_t { mut, imut } mut_modifier;
    std::string_view name;
    type_annotation ty;
    expr_ptr initializer;
};

struct let_data
{
    var_decl decl;
};

struct expr_stmt_data{ expr_ptr expr; };

struct stmt
{
    location src_loc;
    using data_t = std::variant< ret_data,
                    if_data,
                    for_data,
                    while_data,
                    do_while_data,
                    cont_data,
                    brk_data,
                    block_data,
                    let_data,
                    expr_stmt_data >;
    data_t data;
};

struct enum_member
{
    // TODO;
};

struct enum_decl
{
    std::string_view name;
    std::vector< enum_member > members;
};

struct struct_field
{
    std::string_view name;
    type_annotation ty;
};

struct struct_decl
{
    std::string_view name;
    std::vector< struct_field > fields;
};

struct param
{
    std::string_view name;
    type_annotation ty;
};

struct param_list
{
    std::vector< param > params;
};

struct fn_decl
{
    std::string_view name;
    param_list params;
    type_annotation ret_ty;
    std::vector< std::string_view > generics;
    stmt_ptr body;
};

struct global_var
{
    var_decl decl;
};

struct toplevel
{
    location loc;
    using data_t = std::variant< fn_decl, enum_decl, struct_decl, global_var >;
    data_t data;
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
