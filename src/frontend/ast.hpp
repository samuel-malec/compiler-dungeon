#pragma once

#include <variant>
#include <vector>
#include <optional>

#include "token.hpp"
#include "../sema/operations.hpp"

namespace dungeon::ast {
    struct stmt;
    struct expr;
    struct toplevel;

    using expr_ptr = std::unique_ptr<expr>;
    using stmt_ptr = std::unique_ptr<stmt>;
    using toplevel_ptr = std::unique_ptr<toplevel>;

    struct num_lit_data {
        uint64_t value;
    };

    struct float_lit_data {
        double value;
    };

    struct bool_lit_data {
        bool value;
    };

    struct string_lit_data {
        std::string_view value;
    };

    struct identifier_data {
        std::string_view name;
    };

    struct unary_data {
        op_kind op;
        expr_ptr lhs;
    };

    struct binary_data {
        op_kind op;
        expr_ptr lhs;
        expr_ptr rhs;
    };

    struct relational_data {
        op_kind op;
        expr_ptr lhs;
        expr_ptr rhs;
    };

    // ehmmm, the lhs should probably be expr_ptr and we shall deal with invalid assignments in the semantic analyzer
    struct assign_data {
        identifier_data id;
        expr_ptr val;
    };

    struct call_data {
        expr_ptr callee;
        std::vector<expr_ptr> args;
    };

    struct field_access_data {
        expr_ptr object;
        std::string_view field;
    };

    struct array_index_data {
        expr_ptr array;
        expr_ptr index;
    };

    struct if_data {
        expr_ptr cond;
        expr_ptr then_body;
        expr_ptr else_body;
    };

    struct while_data {
        expr_ptr cond;
        expr_ptr body;
    };

    struct loop_data {
        expr_ptr body;
    };

    struct match_arm {
        expr_ptr pattern; // TODO: should be a proper pattern type
        expr_ptr expr;
    };

    struct match_data {
        expr_ptr expr;
        std::vector<match_arm> arms;
    };

    struct struct_literal_field {
        std::string_view name;
        expr_ptr value;
    };

    struct struct_literal_data {
        std::string_view name;
        std::vector<struct_literal_field> fields;
    };

    struct block_data {
        std::vector<stmt_ptr> stmts;
        expr_ptr trailing;
    };

    struct expr {
        src_location src_loc;
        using data_t = std::variant<
            num_lit_data,
            float_lit_data,
            bool_lit_data,
            string_lit_data,
            identifier_data,
            unary_data,
            binary_data,
            relational_data,
            assign_data,
            call_data,
            field_access_data,
            array_index_data,
            if_data,
            while_data,
            loop_data,
            match_data,
            struct_literal_data,
            block_data
        >;
        data_t data;
    };

    struct type_annotation {
        std::string_view base_name;
        std::vector<type_annotation> generic_args;
        bool is_builtin = false;
        bool is_reference = false;
        bool is_mutable_ref = false;
        bool is_array = false;
        bool is_nullable = false;
    };

    struct ret_data {
        expr_ptr val;
    };

    struct cont_data {
    };

    struct brk_data {
    };

    struct var_decl {
        enum mod_t { mut, imut } modifier;
        enum stor_t { local, global } storage;
        std::string_view name;
        std::optional<type_annotation> ty;
        expr_ptr initializer;
    };

    struct let_data {
        var_decl decl;
    };

    struct expr_stmt_data {
        expr_ptr expr;
    };

    struct stmt {
        src_location src_loc;
        using data_t = std::variant<
            ret_data,
            cont_data,
            brk_data,
            let_data,
            expr_stmt_data>;
        data_t data;
    };

    struct enum_member {
        std::string_view name;
        std::vector<type_annotation> data_types;
    };

    struct enum_decl {
        std::string_view name;
        std::vector<std::string_view> generics;
        std::vector<enum_member> members;
    };

    struct struct_field {
        std::string_view name;
        type_annotation ty;
    };

    struct struct_decl {
        std::string_view name;
        std::vector<std::string_view> generics;
        std::vector<struct_field> fields;
    };

    struct param {
        std::string_view name;
        type_annotation ty;
    };

    struct param_list {
        std::vector<param> params;
    };

    struct fn_decl {
        std::string_view name;
        param_list param_list;
        type_annotation ret_ty;
        std::vector<std::string_view> generics;
        expr_ptr body; // the body is just the outermost function block
    };

    struct global_var_decl {
        var_decl decl;
    };

    struct toplevel {
        src_location loc;
        using data_t = std::variant<fn_decl, enum_decl, struct_decl, global_var_decl>;
        data_t data;
    };

    struct module {
        std::vector<toplevel> toplevel_items;
    };

    struct program {
        std::vector<module> modules;
    };
} // namespace dungeon
