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

// TODO: hopefully improve this;
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

struct let_data
{
    enum mut_t { mut, imut } mut_modifier;
    enum scope_t { loc, stat } scope_modifier;
    std::string_view name;
    type_annotation ty;
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

struct enum_decl
{
    std::string_view id;
    std::vector< std::string_view > fields;
};

struct field_decl
{
    std::string_view name;
    type_annotation ty;
};

struct struct_decl
{
    std::string_view name;
    std::vector< field_decl > fields;
};

struct param
{
    std::string_view name;
    type_annotation ty;
};

struct fn_decl
{
    std::string_view name;
    std::vector< stmt_ptr > body;
    std::vector< param > params;
    type_annotation ret_ty;
};

struct toplevel
{
    location loc;
    using data_t =  std::variant< fn_decl, enum_decl, struct_decl >;
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
