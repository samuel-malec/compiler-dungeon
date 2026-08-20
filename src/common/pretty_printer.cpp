#include <optional>

#include "pretty_printer.hpp"
#include "../frontend/ast.hpp"

namespace dungeon::print {
    using atom_map = std::map<uint32_t, std::string>;

    std::string_view op_to_str(op_kind op) {
        switch (op) {
            case ADD: return "+";
            case SUB: return "-";
            case MUL: return "*";
            case DIV: return "/";
            case MOD: return "%";
            case LT: return "<";
            case LEQ: return "<=";
            case GT: return ">";
            case GEQ: return ">=";
            case EQ: return "==";
            case NEQ: return "!=";
            case AND: return "&&";
            case OR: return "||";
            case NOT: return "!";
            case ADD_EQ: return "+=";
            case SUB_EQ: return "-=";
            case MUL_EQ: return "*=";
            case DIV_EQ: return "/=";
            case MOD_EQ: return "%=";
            default: return "<?op?>";
        }
    }

    void print_type_annotation(std::ostream &out, const ast::type_annotation &ty) {
        out << ty.base_name;
    }

    void dungeon::print::pretty_printer::print_expr(std::ostream &out, expr &e, int depth) {
        pad(out, depth);

        if (auto t = std::get_if<ast::num_lit_data>(&e.data)) {
            out << "NumLit " << t->value << '\n';
            return;
        }
        if (auto t = std::get_if<ast::bool_lit_data>(&e.data)) {
            out << "BoolLit " << (t->value ? "true" : "false") << '\n';
            return;
        }
        if (auto t = std::get_if<ast::identifier_data>(&e.data)) {
            out << "Identifier " << t->id << '\n';
            return;
        }
        if (auto t = std::get_if<ast::unary_data>(&e.data)) {
            out << "Unary '" << op_to_str(t->op) << "'\n";
            if (t->lhs)
                print_expr(out, *t->lhs, depth + 1);
            return;
        }
        if (auto t = std::get_if<ast::binary_data>(&e.data)) {
            out << "Binary '" << op_to_str(t->op) << "'\n";
            if (t->lhs)
                print_expr(out, *t->lhs, depth + 1);
            if (t->rhs)
                print_expr(out, *t->rhs, depth + 1);
            return;
        }
        if (auto t = std::get_if<ast::relational_data>(&e.data)) {
            out << "Relational '" << op_to_str(t->op) << "'\n";
            if (t->lhs)
                print_expr(out, *t->lhs, depth + 1);
            if (t->rhs)
                print_expr(out, *t->rhs, depth + 1);
            return;
        }
        if (auto t = std::get_if<ast::assign_data>(&e.data)) {
            out << "Assign " << t->id.id << '\n';
            if (t->val)
                print_expr(out, *t->val, depth + 1);
            return;
        }
        if (auto t = std::get_if<ast::call_data>(&e.data)) {
            out << "Call\n";
            if (t->callee) {
                pad(out, depth + 1);
                out << "callee:\n";
                print_expr(out, *t->callee, depth + 2);
            }
            if (!t->args.empty()) {
                pad(out, depth + 1);
                out << "args:\n";
                for (auto &arg: t->args)
                    if (arg)
                        print_expr(out, *arg, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::float_lit_data>(&e.data)) {
            out << "FloatLit " << t->value << '\n';
            return;
        }
        if (auto t = std::get_if<ast::string_lit_data>(&e.data)) {
            out << "StringLit \"" << t->value << "\"\n";
            return;
        }
        if (auto t = std::get_if<ast::field_access_data>(&e.data)) {
            out << "FieldAccess ." << t->field << "\n";
            if (t->object)
                print_expr(out, *t->object, depth + 1);
            return;
        }
        if (auto t = std::get_if<ast::array_index_data>(&e.data)) {
            out << "ArrayIndex\n";
            if (t->array) {
                pad(out, depth + 1);
                out << "array:\n";
                print_expr(out, *t->array, depth + 2);
            }
            if (t->index) {
                pad(out, depth + 1);
                out << "index:\n";
                print_expr(out, *t->index, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::if_data>(&e.data)) {
            out << "If\n";
            if (t->cond) {
                pad(out, depth + 1);
                out << "cond:\n";
                print_expr(out, *t->cond, depth + 2);
            }
            if (t->then_body) {
                pad(out, depth + 1);
                out << "then:\n";
                print_expr(out, *t->then_body, depth + 2);
            }
            if (t->else_body) {
                pad(out, depth + 1);
                out << "else:\n";
                print_expr(out, *t->else_body, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::while_data>(&e.data)) {
            out << "While\n";
            if (t->cond) {
                pad(out, depth + 1);
                out << "cond:\n";
                print_expr(out, *t->cond, depth + 2);
            }
            if (t->body) {
                pad(out, depth + 1);
                out << "body:\n";
                print_expr(out, *t->body, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::loop_data>(&e.data)) {
            out << "Loop\n";
            if (t->body) {
                pad(out, depth + 1);
                out << "body:\n";
                print_expr(out, *t->body, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::match_data>(&e.data)) {
            out << "Match\n";
            if (t->expr) {
                pad(out, depth + 1);
                out << "expr:\n";
                print_expr(out, *t->expr, depth + 2);
            }
            if (!t->arms.empty()) {
                pad(out, depth + 1);
                out << "arms:\n";
                for (auto &arm: t->arms) {
                    if (arm.pattern) {
                        pad(out, depth + 2);
                        out << "pattern:\n";
                        print_expr(out, *arm.pattern, depth + 3);
                    }
                    if (arm.expr) {
                        pad(out, depth + 2);
                        out << "expr:\n";
                        print_expr(out, *arm.expr, depth + 3);
                    }
                }
            }
            return;
        }
        if (auto t = std::get_if<ast::struct_literal_data>(&e.data)) {
            out << "StructLiteral " << t->name << "\n";
            for (auto &field: t->fields) {
                pad(out, depth + 1);
                out << field.name << ":\n";
                if (field.value)
                    print_expr(out, *field.value, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::block_data>(&e.data)) {
            out << "Block\n";
            for (auto &st: t->stmts)
                if (st)
                    print_stmt(out, *st, depth + 1);
            if (t->trailing) {
                pad(out, depth + 1);
                out << "trailing:\n";
                print_expr(out, *t->trailing, depth + 2);
            }
            return;
        }

        out << "<unknown expr>\n";
    }

    void dungeon::print::pretty_printer::print_stmt(std::ostream &out, stmt &s, int depth) {
        pad(out, depth);
        if (auto t = std::get_if<ast::ret_data>(&s.data)) {
            out << "Return\n";
            if (t->val)
                print_expr(out, *t->val, depth + 1);
            return;
        }
        if (std::get_if<ast::cont_data>(&s.data)) {
            out << "Continue\n";
            return;
        }
        if (std::get_if<ast::brk_data>(&s.data)) {
            out << "Break\n";
            return;
        }
        if (auto t = std::get_if<ast::let_data>(&s.data)) {
            out << "Let " << t->decl.name
                    << (t->decl.mut_modifier == ast::var_decl::mut_t::mut ? " (mut)" : " (imut)")
                    << " : ";
            if (t->decl.ty)
                print_type_annotation(out, *t->decl.ty);
            else
                out << "<inferred>";
            out << "\n";
            if (t->decl.initializer) {
                pad(out, depth + 1);
                out << "init:\n";
                print_expr(out, *t->decl.initializer, depth + 2);
            }
            return;
        }
        if (auto t = std::get_if<ast::expr_stmt_data>(&s.data)) {
            out << "ExprStmt\n";
            if (t->expr)
                print_expr(out, *t->expr, depth + 1);
            return;
        }

        out << "<unknown stmt>\n";
    }

    void dungeon::print::pretty_printer::print_ast_module(std::ostream &out, ast::module &ast_module) {
        for (auto &toplevel: ast_module.toplevel_items) {
            if (auto t = std::get_if<ast::fn_decl>(&toplevel.data)) {
                out << "Fn " << t->name;
                if (!t->generics.empty()) {
                    pad(out, 1);
                    out << "<";
                    for (size_t i = 0; i < t->generics.size(); ++i) {
                        if (i)
                            out << ", ";
                        out << t->generics[i];
                    }
                    out << ">";
                }
                out << "(";
                for (size_t i = 0; i < t->params.params.size(); ++i) {
                    if (i)
                        out << ", ";
                    out << t->params.params[i].name << ": ";
                    print_type_annotation(out, t->params.params[i].ty);
                }
                out << ") -> ";
                if (t->ret_ty) {
                    print_type_annotation(out, *t->ret_ty);
                } else {
                    out << "<inferred>";
                }
                out << '\n';
                pad(out, 1);
                if (t->body)
                    print_expr(out, *t->body, 1);
                continue;
            }
            if (auto t = std::get_if<ast::struct_decl>(&toplevel.data)) {
                out << "Struct " << t->name << '\n';
                for (auto &f: t->fields) {
                    pad(out, 1);
                    out << f.name << ": ";
                    print_type_annotation(out, f.ty);
                    out << '\n';
                }
                continue;
            }
            if (auto t = std::get_if<ast::enum_decl>(&toplevel.data)) {
                out << "Enum " << t->name << '\n';
                continue;
            }
            if (auto t = std::get_if<ast::global_var>(&toplevel.data)) {
                out << "Static " << t->name
                        << (t->is_mutable ? " (mut)" : " (imut)")
                        << " : ";
                print_type_annotation(out, t->ty);
                out << '\n';
                pad(out, 1);
                if (t->initializer)
                    print_expr(out, *t->initializer, 1);
                continue;
            }

            out << "<unknown toplevel>\n";
        }
    }

    void dungeon::print::pretty_printer::print_hir_expr(hir::expr &e, int depth, const atom_map &am) {
    }

    void dungeon::print::pretty_printer::print_hir_stmt(hir::stmt &s, int depth, const atom_map &am) {
    }

    void dungeon::print::pretty_printer::print_hir(hir::program &hir, const atom_map &am) {
    }

    std::string dungeon::print::pretty_printer::tac_operand_to_string(tac::operand &operand, const atom_map &am) {
        return std::visit([ this, am ](auto &&value) -> std::string {
            using T = std::decay_t<decltype( value )>;
            if constexpr (std::is_same_v<T, tac::value>)
                return tac_val_to_string(value, am);
            else {
                std::ostringstream out;
                if (std::holds_alternative<uint64_t>(value))
                    out << std::get<uint64_t>(value);
                if (std::holds_alternative<bool>(value))
                    out << (std::get<bool>(value) ? "true" : "false");
                return out.str();
            }
        }, operand);
    }

    std::string dungeon::print::pretty_printer::tac_instr_symbolic(tac::instr &i, const atom_map &am) {
        return std::visit([ this, am ](auto &&value) -> std::string {
            using T = std::decay_t<decltype( value )>;
            std::ostringstream out;

            if constexpr (std::is_same_v<T, tac::unary_data>) {
                out << tac_val_to_string(value.target, am) << " ← "
                        << value.op << " "
                        << tac_operand_to_string(value.arg1, am);
            } else if constexpr (std::is_same_v<T, tac::binary_data>) {
                out << tac_val_to_string(value.target, am) << " ← "
                        << tac_operand_to_string(value.arg1, am) << " "
                        << value.op << " "
                        << tac_operand_to_string(value.arg2, am);
            } else if constexpr (std::is_same_v<T, tac::copy_data>) {
                out << tac_val_to_string(value.target, am) << " ← "
                        << tac_operand_to_string(value.arg1, am);
            } else if constexpr (std::is_same_v<T, tac::jump_data>) {
                out << "jump ";
                out << value.label;
            } else if constexpr (std::is_same_v<T, tac::branch_data>) {
                out << "branch " << tac_operand_to_string(value.arg1, am)
                        << "  " << value.true_lab << " " << value.false_lab;
            } else if constexpr (std::is_same_v<T, tac::param_data>) {
                out << "param ";
                out << tac_operand_to_string(value.arg, am);
            } else if constexpr (std::is_same_v<T, tac::get_param_data>) {
                out << tac_val_to_string(value.target, am) << " ← get_param "
                        << value.idx;
            } else if constexpr (std::is_same_v<T, tac::call_data>) {
                out << tac_val_to_string(value.target, am) << " ← call "
                        << value.callee << "(" << value.args << " args)";
            } else if constexpr (std::is_same_v<T, tac::label_data>) {
                out << value.id << ":";
            } else if constexpr (std::is_same_v<T, tac::ret_data>) {
                out << "ret";
                if (value.arg.has_value())
                    out << " " << tac_operand_to_string(value.arg.value(), am);
            }

            return out.str();
        }, i.data);
    }


    void dungeon::print::pretty_printer::print_tac(tac::program &tac, const atom_map &am) {
        for (auto &fn: tac.functions) {
            for (auto &i: fn.body)
                print_tac_inst(std::cout, i, am);
        }
    }

    std::string dungeon::print::pretty_printer::value_to_string(const tac::value &v, const atom_map &am) {
        std::ostringstream os;
        os << "v" << v.id << "." << v.version;
        return os.str();
    }

    std::string dungeon::print::pretty_printer::phi_to_string(const cfg::phi_node &phi, const atom_map &am) {
        std::ostringstream os;
        os << value_to_string(phi.res, am) << " = φ(";
        bool first = true;
        for (auto &[pred_id, val]: phi.incoming) {
            if (!first) os << ", ";
            os << "bb" << pred_id << ": " << value_to_string(val, am);
            first = false;
        }
        os << ")";
        return os.str();
    }

    void dungeon::print::pretty_printer::export_to_dot(cfg::cfg &graph, std::ostream &out, const atom_map &am) {
        out << "digraph CFG {\n";
        out << "    node [shape=box, fontname=\"Courier New\", fontsize=10, style=filled, fillcolor=\"#f9f9f9\"];\n";
        out << "    edge [fontname=\"Courier New\", fontsize=9];\n\n";

        if (!graph.basic_blocks.empty()) {
            out <<
                    "    entry [shape=circle, label=\"entry\", style=filled, fillcolor=\"#d4edda\", fontname=\"Courier New\", fontsize=10, width=0.5, fixedsize=true];\n";
            out << "    entry -> block_" << graph.basic_blocks.front()->id << ";\n\n";
        }

        for (const auto &bb: graph.basic_blocks) {
            out << "    block_" << bb->id << " [label=\"";
            out << "BB " << bb->id << "\\n";
            out << "--------------------------------\\n";

            if (!bb->phis.empty()) {
                for (auto &phi: bb->phis) {
                    std::string s = phi_to_string(phi, am);
                    size_t pos = 0;
                    while ((pos = s.find('"', pos)) != std::string::npos) {
                        s.replace(pos, 1, "\\\"");
                        pos += 2;
                    }
                    out << s << "\\n";
                }
                out << "................................\\n";
            }

            for (auto &ins: bb->instructions) {
                std::string inst_str = "";

                if (std::holds_alternative<tac::branch_data>(ins.data)) {
                    std::ostringstream os;
                    os << "branch ";
                    auto bd = std::get<tac::branch_data>(ins.data);
                    os << tac_operand_to_string(bd.arg1, am);
                    inst_str = os.str();
                } else if (std::holds_alternative<tac::jump_data>(ins.data))
                    inst_str = "jump";
                else
                    inst_str = tac_instr_symbolic(ins, am);

                size_t pos = 0;
                while ((pos = inst_str.find('"', pos)) != std::string::npos) {
                    inst_str.replace(pos, 1, "\\\"");
                    pos += 2;
                }
                out << inst_str << "\\n";
            }
            out << "\"];\n";
        }

        out << "\n";

        for (const auto &bb: graph.basic_blocks) {
            bool is_conditional = false;
            if (!bb->instructions.empty()) {
                const auto &term = bb->instructions.back();
                if (std::holds_alternative<tac::branch_data>(term.data)) {
                    is_conditional = true;
                }
            }

            for (size_t i = 0; i < bb->succ.size(); ++i) {
                const auto succ = bb->succ[i];
                if (succ) {
                    out << "    block_" << bb->id << " -> block_" << succ->id;

                    if (is_conditional) {
                        if (i == 0)
                            out << " [label=\"true\", color=\"#2ca02c\", fontcolor=\"#2ca02c\"]"; // Forest Green
                        else if (i == 1)
                            out << " [label=\"false\", color=\"#d62728\", fontcolor=\"#d62728\"]"; // Crimson Red
                    }
                    out << ";\n";
                }
            }
        }

        out << "}\n";
    }

    void pretty_printer::print_tokens(const std::vector<token> &toks) {
        for (auto &t: toks)
            std::cout << t << "\n";
    }
}
