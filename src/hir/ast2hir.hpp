#pragma once

#include "../frontend/ast.hpp"

namespace dungeon::hir {
    struct fn_builder {
        function fn;

        expr_id add_expr(const type *ty, expr::data_t data) {
            expr_id curr{.idx = static_cast<uint32_t>(fn.exprs.size())};
            expr e{.ty = ty, .data = std::move(data)};
            fn.exprs.push_back(e);
            return curr;
        }

        stmt_id add_stmt(stmt::data_t data) {
            stmt_id curr{.idx = static_cast<uint32_t>(fn.stmts.size())};
            stmt s{.data = std::move(data)};
            fn.stmts.push_back(s);
            return curr;
        }


        stmt_id lower_stmt_to_hir(ast::stmt &stmt, const sema::analysis_result &sema) {
            if (auto ld = std::get_if<ast::let_data>(&stmt.data)) {
                sema::symbol_id sid = sema.var_decl_symbols.at(&ld->decl);
                return add_stmt(stmt::let_data{.target = sid, .value = lower_expr_to_hir(*ld->decl.initializer, sema)});
            }
            if (auto exp = std::get_if<ast::expr_stmt_data>(&stmt.data)) {
                return add_stmt(stmt::expr_data{.e = lower_expr_to_hir(*exp->expr, sema)});
            }
            if (auto rd = std::get_if<ast::ret_data>(&stmt.data)) {
                stmt::ret_data hrd{.value = std::nullopt};
                if (rd->val)
                    hrd.value = lower_expr_to_hir(*rd->val, sema);
                return add_stmt(hrd);
            }
            if (std::get_if<ast::brk_data>(&stmt.data)) {
                return add_stmt(stmt::brk{});
            }
            if (std::get_if<ast::cont_data>(&stmt.data)) {
                return add_stmt(stmt::cont{});
            }

            assert(false && "unreachable");
        }

        expr_id lower_expr_to_hir(ast::expr &expr, const sema::analysis_result &sema) {
            auto ty = sema.expr_ty.at(&expr);
            if (!ty)
                diag::error("Unresolved type at ", expr.src_loc);

            if (auto nld = std::get_if<ast::num_lit_data>(&expr.data)) {
                return add_expr(ty, expr::int_lit{.val = nld->value});
            }
            if (std::get_if<ast::bool_lit_data>(&expr.data)) {
                return add_expr(ty, expr::bool_lit{.val = true});
            }
            if (auto id = std::get_if<ast::identifier_data>(&expr.data)) {
                return add_expr(ty, expr::var_data{.sid = sema.id_symbols.at(&expr)});
            }
            if (auto ud = std::get_if<ast::unary_data>(&expr.data)) {
                auto lhs = lower_expr_to_hir(*ud->lhs, sema);
                return add_expr(ty, expr::unary_data{.op = ud->op, .lhs = lhs});
            }
            if (auto bd = std::get_if<ast::binary_data>(&expr.data)) {
                auto lhs = lower_expr_to_hir(*bd->lhs, sema);
                auto rhs = lower_expr_to_hir(*bd->rhs, sema);
                return add_expr(ty, expr::binary_data{.op = bd->op, .lhs = lhs, .rhs = rhs});
            }
            if (auto rd = std::get_if<ast::relational_data>(&expr.data)) {
                auto lhs = lower_expr_to_hir(*rd->lhs, sema);
                auto rhs = lower_expr_to_hir(*rd->rhs, sema);
                return add_expr(ty, expr::relational_data{.op = rd->op, .lhs = lhs, .rhs = rhs});
            }
            if (auto ad = std::get_if<ast::assign_data>(&expr.data)) {
                sema::symbol_id sid = sema.id_symbols.at(&expr);
                auto val = lower_expr_to_hir(*ad->val, sema);
                return add_expr(ty, expr::assign_data{.target = sid, .value = val});
            }
            if (auto cd = std::get_if<ast::call_data>(&expr.data)) {
                expr::call_data hcd{};
                auto fn = sema.expr_fn.at(&expr);
                hcd.target = fn->id;

                for (auto &arg: cd->args)
                    hcd.args.push_back(lower_expr_to_hir(*arg, sema));
                return add_expr(ty, std::move(hcd));
            }
            if (auto fad = std::get_if<ast::field_access_data>(&expr.data)) {
                // TODO
            }
            if (auto aid = std::get_if<ast::array_index_data>(&expr.data)) {
                // TODO
            }
            if (auto ifd = std::get_if<ast::if_data>(&expr.data)) {
                auto cond = lower_expr_to_hir(*ifd->cond, sema);
                auto then_body = lower_expr_to_hir(*ifd->then_body, sema);

                if (ifd->else_body) {
                    auto else_body = lower_expr_to_hir(*ifd->else_body, sema);
                    return add_expr(ty, expr::if_data{.cond = cond, .then_body = then_body, .else_body = else_body});
                }

                return add_expr(ty, expr::if_data{.cond = cond, .then_body = then_body, .else_body = std::nullopt});
            }
            if (auto wd = std::get_if<ast::while_data>(&expr.data)) {
                auto cond = lower_expr_to_hir(*wd->cond, sema);
                auto body = lower_expr_to_hir(*wd->body, sema);
                return add_expr(ty, expr::while_data{.cond = cond, .body = body});
            }
            if (auto ld = std::get_if<ast::loop_data>(&expr.data)) {
                auto body = lower_expr_to_hir(*ld->body, sema);
                return add_expr(ty, expr::loop_data{.body = body});
            }
            if (auto md = std::get_if<ast::match_data>(&expr.data)) {
                // TODO
            }
            if (auto sd = std::get_if<ast::struct_literal_data>(&expr.data)) {
                // TODO
            }
            if (auto blk = std::get_if<ast::block_data>(&expr.data)) {
                expr::block_data hb{};
                for (auto &s: blk->stmts) {
                    hb.stmts.push_back(lower_stmt_to_hir(*s, sema));
                }
                if (blk->trailing)
                    hb.trailing = lower_expr_to_hir(*blk->trailing, sema);
                return add_expr(ty, std::move(hb));
            }
            assert(false && "unreachable");
        }

        function build(ast::fn_decl &fun, const sema::analysis_result &sema) {
            fn.root = lower_expr_to_hir(*fun.body, sema);
            return std::move(fn);
        }
    };

    hir::module lower_ast_to_hir(ast::module &ast, const sema::analysis_result &sema) {
        hir::module res{};
        for (auto &[loc, data]: ast.toplevel_items) {
            if (const auto fdecl = std::get_if<ast::fn_decl>(&data)) {
                fn_builder builder{};
                function fn = builder.build(*fdecl, sema);
                res.functions.push_back(std::move(fn));
            }
        }
        return res;
    }
}
