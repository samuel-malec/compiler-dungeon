#include <optional>

#include "pretty_printer.hpp"
#include "../frontend/ast.hpp"

namespace dungeon::print {
    std::string_view op_to_str(op_kind op) {
        switch (op) {
            case ADD: return "+";
            case SUB: return "-";
            case MINUS: return "-";
            case PLUS: return "+";
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

    void pretty_printer::print_expr(std::ostream &out, expr &e, int depth) {
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
            out << "Identifier " << t->name << '\n';
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
            out << "Assign " << t->id.name << '\n';
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

    void pretty_printer::print_stmt(std::ostream &out, stmt &s, int depth) {
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
                    << (t->decl.modifier == ast::var_decl::mod_t::mut ? " (mut)" : " (imut)")
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

    void pretty_printer::print_ast_module(std::ostream &out, ast::module &ast_module) {
        std::cout << "AST\n";
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
                print_type_annotation(out, t->ret_ty);
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
            if (auto t = std::get_if<ast::global_var_decl>(&toplevel.data)) {
                // TODO: we can extract the common 'var_decl' printing part from here and from let_data
                out << "Static " << t->decl.name
                        << (t->decl.modifier == ast::var_decl::mut ? " (mut)" : " (imut)")
                        << " : ";
                print_type_annotation(out, t->decl.ty.value());
                out << '\n';
                pad(out, 1);
                if (t->decl.initializer)
                    print_expr(out, *t->decl.initializer, 1);
                continue;
            }

            out << "<unknown toplevel>\n";
        }
    }

    void print_hir_type(std::ostream &out, const dungeon::type *ty) {
        if (!ty) {
            out << "<null type>";
            return;
        }

        switch (ty->kind) {
            case type_kind::_int:
                out << "i" << ty->bits;
                break;

            case type_kind::_uint:
                out << "u" << ty->bits;
                break;

            case type_kind::_bool:
                out << "bool";
                break;

            case type_kind::_unit:
                out << "unit";
                break;
        }
    }

    void pretty_printer::print_hir_expr(
        std::ostream &out,
        const hir::function &fn,
        hir::expr_id id,
        int depth) {
        const auto &e = fn.exprs.at(id.idx);

        pad(out, depth);

        if (auto t = std::get_if<hir::expr::int_lit>(&e.data)) {
            out << "IntLit " << t->val << " : ";
            print_hir_type(out, e.ty);
            out << '\n';
            return;
        }

        if (auto t = std::get_if<hir::expr::bool_lit>(&e.data)) {
            out << "BoolLit "
                    << (t->val ? "true" : "false")
                    << " : ";
            print_hir_type(out, e.ty);
            out << '\n';
            return;
        }

        if (auto t = std::get_if<hir::expr::var_data>(&e.data)) {
            out << "Var #" << t->sid.value << " : ";
            print_hir_type(out, e.ty);
            out << '\n';
            return;
        }

        if (auto t = std::get_if<hir::expr::unary_data>(&e.data)) {
            out << "Unary '" << op_to_str(t->op) << "' : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "operand:\n";
            print_hir_expr(out, fn, t->lhs, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::expr::binary_data>(&e.data)) {
            out << "Binary '" << op_to_str(t->op) << "' : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "lhs:\n";
            print_hir_expr(out, fn, t->lhs, depth + 2);

            pad(out, depth + 1);
            out << "rhs:\n";
            print_hir_expr(out, fn, t->rhs, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::expr::relational_data>(&e.data)) {
            out << "Relational '" << op_to_str(t->op) << "' : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "lhs:\n";
            print_hir_expr(out, fn, t->lhs, depth + 2);

            pad(out, depth + 1);
            out << "rhs:\n";
            print_hir_expr(out, fn, t->rhs, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::expr::assign_data>(&e.data)) {
            out << "Assign #" << t->target.value << " : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "value:\n";
            print_hir_expr(out, fn, t->value, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::expr::call_data>(&e.data)) {
            out << "Call fn#" << t->target.value << " : ";
            print_hir_type(out, e.ty);
            out << '\n';

            if (!t->args.empty()) {
                pad(out, depth + 1);
                out << "args:\n";

                for (auto arg: t->args)
                    print_hir_expr(out, fn, arg, depth + 2);
            }

            return;
        }

        if (auto t = std::get_if<hir::expr::if_data>(&e.data)) {
            out << "If : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "cond:\n";
            print_hir_expr(out, fn, t->cond, depth + 2);

            pad(out, depth + 1);
            out << "then:\n";
            print_hir_expr(out, fn, t->then_body, depth + 2);

            if (t->else_body) {
                pad(out, depth + 1);
                out << "else:\n";
                print_hir_expr(out, fn, *t->else_body, depth + 2);
            }

            return;
        }

        if (auto t = std::get_if<hir::expr::while_data>(&e.data)) {
            out << "While : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "cond:\n";
            print_hir_expr(out, fn, t->cond, depth + 2);

            pad(out, depth + 1);
            out << "body:\n";
            print_hir_expr(out, fn, t->body, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::expr::loop_data>(&e.data)) {
            out << "Loop : ";
            print_hir_type(out, e.ty);
            out << '\n';

            pad(out, depth + 1);
            out << "body:\n";
            print_hir_expr(out, fn, t->body, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::expr::block_data>(&e.data)) {
            out << "Block : ";
            print_hir_type(out, e.ty);
            out << '\n';

            for (auto stmt: t->stmts)
                print_hir_stmt(out, fn, stmt, depth + 1);

            if (t->trailing) {
                pad(out, depth + 1);
                out << "trailing:\n";
                print_hir_expr(out, fn, *t->trailing, depth + 2);
            }

            return;
        }

        out << "<unknown hir expr>\n";
    }

    void pretty_printer::print_hir_stmt(
        std::ostream &out,
        const hir::function &fn,
        hir::stmt_id id,
        int depth) {
        const auto &s = fn.stmts.at(id.idx);

        pad(out, depth);

        if (auto t = std::get_if<hir::stmt::expr_data>(&s.data)) {
            out << "ExprStmt\n";
            print_hir_expr(out, fn, t->e, depth + 1);
            return;
        }

        if (auto t = std::get_if<hir::stmt::let_data>(&s.data)) {
            out << "Let #" << t->target.value << '\n';

            pad(out, depth + 1);
            out << "value:\n";
            print_hir_expr(out, fn, t->value, depth + 2);
            return;
        }

        if (auto t = std::get_if<hir::stmt::ret_data>(&s.data)) {
            out << "Return\n";

            if (t->value) {
                pad(out, depth + 1);
                out << "value:\n";
                print_hir_expr(out, fn, *t->value, depth + 2);
            }

            return;
        }

        if (std::get_if<hir::stmt::brk>(&s.data)) {
            out << "Break\n";
            return;
        }

        if (std::get_if<hir::stmt::cont>(&s.data)) {
            out << "Continue\n";
            return;
        }

        out << "<unknown hir stmt>\n";
    }

    void pretty_printer::print_hir_module(
        const hir::module &hir_module) {
        std::cout << "HIR:\n";

        for (size_t i = 0; i < hir_module.functions.size(); ++i) {
            const auto &fn = hir_module.functions[i];

            std::cout << "Function #" << i << '\n';

            if (fn.root.idx >= fn.exprs.size()) {
                std::cout << "  <invalid root stmt #" << fn.root.idx << ">\n";
                continue;
            }

            print_hir_expr(std::cout, fn, fn.root, 1);
        }
    }

    void pretty_printer::print_tokens(const std::vector<token> &toks) {
        for (auto &t: toks)
            std::cout << t << "\n";
    }
}
