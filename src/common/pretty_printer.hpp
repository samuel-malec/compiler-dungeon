#pragma once

#include <string>

#include "../frontend/ast.hpp"
#include "../sema/semantic.hpp"
#include "../hir/hir.hpp"
#include "../ir/module.hpp"

namespace dungeon::print {
    struct pretty_printer {
        using expr = ast::expr;
        using stmt = ast::stmt;
        using toplevel = ast::toplevel;
        using var_decl = ast::let_data;
        using fn_decl = ast::fn_decl;
        using enum_decl = ast::enum_decl;
        using struct_decl = ast::struct_decl;
        using atom_map = std::map<uint32_t, std::string>;

        std::string indent(std::ostream &out, int depth) {
            return std::string(depth * 2, ' ');
        }

        void pad(std::ostream &out, int depth) {
            out << indent(out, depth);
        }

        void print_expr(std::ostream &out, expr &e, int depth);

        void print_var_decl(std::ostream &out, ast::var_decl &decl, int depth);

        void print_stmt(std::ostream &out, stmt &s, int depth);

        void print_ast_module(std::ostream &out, ast::module &ast_module);

        void print_hir_expr(
            std::ostream &out,
            const hir::function &fn,
            hir::expr_id id,
            int depth);

        void print_hir_stmt(
            std::ostream &out,
            const hir::function &fn,
            hir::stmt_id id,
            int depth);

        void print_hir_module(std::ostream &os, const hir::module &hir_module);

        void print_ir_value(std::ostream &out, const ir::value *value);

        void print_ir_op(std::ostream &out, ir::opcode op);

        void print_ir_operands(std::ostream &out, const std::vector<ir::value *> &operands);

        void print_ir_instruction(std::ostream &out, const ir::instruction *i);

        void print_ir_function(std::ostream &out, const ir::function &func);

        void print_ir_module(std::ostream& out, const ir::module &module);

        void export_to_dot(std::ostream& out, const ir::module & module);

        static void print_tokens(const std::vector<token> &toks);
    };
}
