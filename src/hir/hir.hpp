#pragma once

#include <cassert>
#include <variant>
#include <memory>

#include "../sema/semantic.hpp"
#include "../sema/types.hpp"

namespace dungeon::hir {
    struct expr_id {
        uint32_t idx;
    };

    struct stmt_id {
        uint32_t idx;
    };

    struct expr {
        const type *ty;

        struct int_lit {
            std::uint64_t val;
        };

        struct bool_lit {
            bool val;
        };

        struct var_data {
            sema::symbol_id sid;
        };

        struct unary_data {
            op_kind op;
            expr_id lhs;
        };

        // TODO: split this into binary_arith and binary_logic data ?
        struct binary_data {
            op_kind op;
            expr_id lhs;
            expr_id rhs;
        };

        struct relational_data {
            op_kind op;
            expr_id lhs;
            expr_id rhs;
        };

        struct assign_data {
            sema::symbol_id target;
            expr_id value;
        };

        struct call_data {
            sema::fn_id target;
            std::vector<expr_id> args;
        };

        struct if_data {
            expr_id cond;
            expr_id then_body;
            std::optional<expr_id> else_body;
        };

        struct while_data {
            expr_id cond;
            expr_id body;
        };

        struct loop_data {
            expr_id body;
        };

        struct block_data {
            std::vector<stmt_id> stmts;
            std::optional<expr_id> trailing;
        };

        using data_t = std::variant<
            int_lit,
            bool_lit,
            var_data,
            unary_data,
            binary_data,
            relational_data,
            assign_data,
            call_data,
            if_data,
            while_data,
            loop_data,
            block_data>;
        data_t data;
    };

    struct stmt {
        struct expr_data {
            expr_id e;
        };

        struct let_data {
            sema::symbol_id target;
            expr_id value;
        };

        struct ret_data {
            std::optional<expr_id> value;
        };

        struct brk {
        };

        struct cont {
        };

        using data_t = std::variant<
            expr_data,
            let_data,
            ret_data,
            brk,
            cont>;
        data_t data;
    };

    struct function {
        expr_id root;
        std::vector<sema::symbol_id> params;
        std::vector<expr> exprs;
        std::vector<stmt> stmts;

        const expr &get_expr(int idx) const {
            assert(idx >= 0 && idx < exprs.size());
            return exprs[idx];
        }

        const stmt &get_stmt(int idx) const {
            assert(idx >= 0 && idx < stmts.size());
            return stmts[idx];
        }
    };

    struct module {
        std::vector<function> functions;
    };
} // namespace dungeon::hir
