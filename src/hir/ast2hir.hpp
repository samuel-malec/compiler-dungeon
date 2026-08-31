#pragma once

#include "../frontend/ast.hpp"

namespace dungeon::hir {
    hir::function lower_fn_to_hir(ast::fn_decl &fun, const sema::analysis_result &sema) {
        hir::function res{};
        return res;
    }

    hir::module lower_ast_to_hir(ast::module &ast, const sema::analysis_result &sema) {
        hir::module res{};
        for (auto &[loc, data]: ast.toplevel_items) {
            if (const auto fdecl = std::get_if<ast::fn_decl>(&data)) {
                function fn = lower_fn_to_hir(*fdecl, sema);
                res.functions.push_back(std::move(fn));
            }
        }
        return res;
    }
}
