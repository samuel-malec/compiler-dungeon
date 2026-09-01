#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include "../frontend/ast.hpp"
#include "../sema/semantic.hpp"
#include "../ir/cfg/cfg.hpp"
// #include "../middleend/tac.hpp"
#include "../hir/hir.hpp"

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

        void print_hir_module(const hir::module &hir);

        // std::string tac_operand_to_string(tac::operand &operand, const atom_map &am);
        //
        // std::string tac_instr_symbolic(tac::instr &i, const atom_map &am);
        //
        // void print_tac_inst(std::ostream &out, tac::instr &i, const atom_map &am) {
        //     std::holds_alternative<tac::label_data>(i.data) ? pad(out, 2) : pad(out, 4);
        //     out << tac_instr_symbolic(i, am) << '\n';
        // }
        //
        // void print_tac(tac::program &tac, const atom_map &am);
        //
        // std::string value_to_string(const tac::value &v, const atom_map &am);
        //
        // std::string phi_to_string(const cfg::phi_node &phi, const atom_map &am);
        //
        // void export_to_dot(cfg::cfg &graph, std::ostream &out, const atom_map &am);

        static void print_tokens(const std::vector<token> & toks);
    };
}
