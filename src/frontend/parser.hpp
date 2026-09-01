#pragma once

#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"
#include "../common/diag.hpp"
#include "../sema/types.hpp"

namespace dungeon {
    struct parser {
        using cat = token::cat_t;
        using expr = ast::expr;
        using stmt = ast::stmt;
        using toplevel = ast::toplevel;
        using fn_decl = ast::fn_decl;
        using var_decl = ast::var_decl;
        using enum_decl = ast::enum_decl;
        using struct_decl = ast::struct_decl;
        using global_var = ast::global_var_decl;
        using module = ast::module;

        token current;
        size_t pos = 0;
        std::vector<token> toks;
        bool parsing_control_condition = false;

        parser(std::vector<token> toks) : toks{std::move(toks)} {
        }

        token require(cat c, std::string_view data = "") {
            auto tok = fetch();
            if (tok.cat != c)
                diag::error(tok, "Expected category: ", c, data.empty() ? "" : ", data: ", data);

            if (!data.empty() && tok.data != data)
                diag::error(tok, "Expected: ", data);

            return tok;
        }

        bool match(cat c, std::string_view data = "") {
            auto tok = peek();
            if (tok.cat != c)
                return false;

            if (data.empty())
                return true;

            return tok.data == data;
        }

        template<typename... Args>
        std::optional<token> match_any(cat c, Args... args) {
            auto tok = peek();
            if (tok.cat != c)
                return {};

            if (( (tok.data == args) || ... ))
                return tok;

            return {};
        }

        token peek() {
            if (pos >= toks.size())
                return token{.data = "<eof>", .cat = cat::invalid};
            return toks[pos];
        }

        token fetch() {
            auto rv = peek();
            if (pos < toks.size())
                ++pos;
            return rv;
        }

        bool empty() const {
            return pos >= toks.size();
        }

        ast::expr make_compound_assignment(ast::expr &&lhs, ast::expr &&rhs, std::string_view op) {
            auto *v = std::get_if<ast::identifier_data>(&lhs.data);
            if (!v)
                diag::error("Left-hand side of compound assignment must be an identifier");

            ast::identifier_data id = *v;
            src_location loc = lhs.src_loc;
            std::string_view base_op = op.substr(0, op.size() - 1);
            ast::expr bin = make_binary(std::move(lhs), std::move(rhs), op_kind_from_str(base_op));

            return ast::expr{
                .src_loc = loc,
                .data = ast::assign_data{
                    .id = id,
                    .val = make_expr(std::move(bin)),
                }
            };
        }

        ast::expr make_relational(expr &&lhs, expr &&rhs, op_kind op) {
            ast::relational_data bd{};
            bd.lhs = make_expr(std::move(lhs));
            bd.rhs = make_expr(std::move(rhs));
            bd.op = op;
            return expr{.src_loc = bd.lhs->src_loc, .data = std::move(bd)};
        }

        ast::expr make_binary(expr &&lhs, expr &&rhs, op_kind op) {
            ast::binary_data bd{};
            bd.lhs = make_expr(std::move(lhs));
            bd.rhs = make_expr(std::move(rhs));
            bd.op = op;
            return expr{.src_loc = bd.lhs->src_loc, .data = std::move(bd)};
        }

        static ast::stmt_ptr make_stmt(stmt &&s) {
            return std::make_unique<stmt>(std::move(s));
        }

        static ast::expr_ptr make_expr(expr &&e) {
            return std::make_unique<expr>(std::move(e));
        }

        static bool may_omit_statement_semicolon(const expr &e) {
            return std::holds_alternative<ast::if_data>(e.data) ||
                   std::holds_alternative<ast::while_data>(e.data) ||
                   std::holds_alternative<ast::loop_data>(e.data);
        }

        std::optional<expr> parse_primary();

        std::optional<expr> parse_postfix();

        std::optional<expr> parse_unary();

        std::optional<expr> parse_multiplicative();

        std::optional<expr> parse_additive();

        std::optional<expr> parse_relational();

        std::optional<expr> parse_equality();

        std::optional<expr> parse_assignment();

        std::optional<expr> parse_and();

        std::optional<expr> parse_or();

        std::optional<expr> parse_expr();

        std::optional<expr> parse_block();

        std::optional<expr> parse_if_expr();

        std::optional<expr> parse_match_expr();

        std::optional<expr> parse_pattern();

        std::optional<expr> parse_loop_expr();

        std::optional<expr> parse_while_expr();

        std::optional<stmt> parse_expr_stmt();

        std::optional<stmt> parse_ret();

        std::optional<stmt> parse_control_stmt();

        std::optional<stmt> parse_let();

        std::optional<ast::type_annotation> parse_type_annotation();

        std::optional<ast::param> parse_param();

        std::optional<ast::param_list> parse_param_list();

        std::optional<stmt> parse_stmt();

        std::optional<ast::var_decl> parse_var_decl();

        std::optional<toplevel> parse_global_var_decl();

        std::vector<std::string_view> parse_generic_params();

        std::optional<toplevel> parse_fn_decl();

        std::vector<ast::enum_member> parse_members();

        std::optional<toplevel> parse_enum_decl();

        std::optional<toplevel> parse_struct_decl();

        std::optional<toplevel> parse_toplevel();

        std::optional<module> parse_module();
    };
}
