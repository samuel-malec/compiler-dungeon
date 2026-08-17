#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include "../frontend/ast.hpp"
#include "../sema/semantic.hpp"
#include "../middleend/cfg.hpp"
#include "../middleend/tac.hpp"
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

        std::string indent(int depth) {
            return std::string(depth * 2, ' ');
        }

        void pad(int depth) {
            std::cout << indent(depth);
        }

        std::string tac_val_to_string(const tac::value &v, const atom_map &am) {
            return "v" + std::to_string(v.id) + "." + std::to_string(v.version);
        }

        void print_expr(std::ostream &out, expr &e, int depth);

        void print_stmt(std::ostream &out, stmt &s, int depth);

        void print_ast_module(std::ostream &out, ast::module &ast_module);

        void print_hir_expr(hir::expr &e, int depth, const atom_map &am);

        void print_hir_stmt(hir::stmt &s, int depth, const atom_map &am);

        void print_hir(hir::program &hir, const atom_map &am);

        std::string tac_operand_to_string(tac::operand &operand, const atom_map &am);

        std::string tac_instr_symbolic(tac::instr &i, const atom_map &am);

        void print_tac_inst(tac::instr &i, const atom_map &am) {
            std::holds_alternative<tac::label_data>(i.data) ? pad(2) : pad(4);
            std::cout << tac_instr_symbolic(i, am) << '\n';
        }

        void print_tac(tac::program &tac, const atom_map &am);

        void export_to_dot(cfg::cfg &graph, std::ostream &out, const atom_map &am);
    };
}
