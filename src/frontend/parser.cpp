#include "parser.hpp"

namespace dungeon {
    std::optional<ast::expr> parser::parse_primary() {
        if (match(cat::punct, "(")) {
            fetch();
            auto e = parse_expr();
            require(cat::punct, ")");
            return e;
        }

        if (match(cat::punct, "{")) {
            auto b = parse_block();
            if (!b) diag::error("Expected block");
            return b;
        }

        if (match(cat::keyword, "if")) {
            return parse_if_expr();
        }

        if (match(cat::keyword, "match")) {
            return parse_match_expr();
        }

        if (match(cat::keyword, "loop")) {
            return parse_loop_expr();
        }

        if (match(cat::keyword, "while")) {
            return parse_while_expr();
        }

        if (match(cat::string)) {
            auto tok = fetch();
            return ast::expr{.src_loc = tok.loc,
                .data = ast::string_lit_data{.value = tok.data.substr(1, tok.data.size() - 2)}};
        }

        if (match(cat::number)) {
            auto tok = fetch();
            if (tok.data.find('.') != std::string::npos) {
                double n{};
                auto [p, ec] = std::from_chars(tok.data.data(), tok.data.data() + tok.data.size(), n);
                if (ec != std::errc() || p != tok.data.data() + tok.data.size())
                    diag::error(tok, "Invalid float literal");
                return ast::expr{.src_loc = tok.loc, .data = ast::float_lit_data{.value = n}};
            }
            uint64_t n{};
            auto [p, ec] = std::from_chars(tok.data.data(), tok.data.data() + tok.data.size(), n);
            if (ec != std::errc() || p != tok.data.data() + tok.data.size())
                diag::error(tok, "Invalid numeric literal");
            return ast::expr{.src_loc = tok.loc, .data = ast::num_lit_data{.value = n}};
        }

        if (auto t = match_any(cat::keyword, "true", "false")) {
            auto tok = fetch();
            bool value = t.value().data == "true";
            return ast::expr{.src_loc = tok.loc, .data = ast::bool_lit_data{.value = value}};
        }

        if (match(cat::ident)) {
            auto tok = fetch();

            // A block immediately following an identifier can be the body of
            // `if`/`while`, rather than a struct literal. A struct literal is
            // unambiguous once its first field (`name:`) is visible.
            bool looks_like_struct_literal =
                !parsing_control_condition && match(cat::punct, "{") &&
                ((pos + 1 < toks.size() && toks[pos + 1].cat == cat::punct && toks[pos + 1].data == "}") ||
                 (pos + 2 < toks.size() && toks[pos + 1].cat == cat::ident &&
                  toks[pos + 2].cat == cat::punct && toks[pos + 2].data == ":"));
            if (looks_like_struct_literal) {
                fetch();
                std::vector<ast::struct_literal_field> fields;

                if (!match(cat::punct, "}")) {
                    while (true) {
                        auto field_name = require(cat::ident).data;
                        require(cat::punct, ":");
                        auto field_expr = parse_expr();
                        if (!field_expr) diag::error("Expected expression in struct field");

                        fields.push_back(ast::struct_literal_field{
                            .name = field_name,
                            .value = make_expr(std::move(field_expr.value()))
                        });

                        if (!match(cat::punct, ",")) break;
                        fetch();
                        if (match(cat::punct, "}")) break; // trailing comma
                    }
                }

                require(cat::punct, "}");
                return ast::expr{
                    .src_loc = tok.loc, .data = ast::struct_literal_data{
                        .name = tok.data,
                        .fields = std::move(fields)
                    }
                };
            }

            return ast::expr{.src_loc = tok.loc, .data = ast::identifier_data{.id = tok.data}};
        }

        return {};
    }

    std::optional<ast::expr> parser::parse_postfix() {
        auto e = parse_primary();
        if (!e)
            return {};

        while (true) {
            if (match(cat::punct, "(")) {
                location loc = e->src_loc;
                fetch();

                ast::call_data cd{};
                cd.callee = make_expr(std::move(*e));

                if (!match(cat::punct, ")")) {
                    while (true) {
                        auto arg = parse_expr();
                        if (!arg)
                            diag::error("Expected function argument");

                        cd.args.push_back(make_expr(std::move(arg.value())));
                        if (!match(cat::punct, ","))
                            break;
                        fetch();
                    }
                }

                require(cat::punct, ")");
                e = ast::expr{.src_loc = loc, .data = std::move(cd)};
            } else if (match(cat::punct, ".")) {
                fetch();
                auto field_name = require(cat::ident).data;
                e = ast::expr{
                    .src_loc = e->src_loc,
                    .data = ast::field_access_data{
                        .object = make_expr(std::move(*e)),
                        .field = field_name
                    }
                };
            } else if (match(cat::punct, "[")) {
                fetch();
                auto index = parse_expr();
                if (!index)
                    diag::error("Expected index expression");
                require(cat::punct, "]");
                e = ast::expr{
                    .src_loc = e->src_loc,
                    .data = ast::array_index_data{
                        .array = make_expr(std::move(*e)),
                        .index = make_expr(std::move(index.value()))
                    }
                };
            } else {
                break;
            }
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_unary() {
        if (match(cat::punct, "&")) {
            fetch();
            if (match(cat::keyword, "mut")) {
                bool is_mut = false;
                fetch();
                is_mut = true;
            }
            auto rhs = parse_unary();
            if (!rhs)
                diag::error("Expected unary operand after &");
            // TODO: Create a proper reference expression type
            return rhs;
        }

        if (auto t = match_any(cat::punct, "!", "-", "+")) {
            fetch();
            auto rhs = parse_unary();
            if (!rhs)
                diag::error("Expected unary operand");

            ast::unary_data ud{};
            ud.lhs = make_expr(std::move(rhs.value()));
            ud.op = op_kind_from_str(t->data);
            return ast::expr{.src_loc = t->loc, .data = std::move(ud)};
        }

        return parse_postfix();
    }

    std::optional<ast::expr> parser::parse_multiplicative() {
        auto e = parse_unary();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "/", "*", "%")) {
            fetch();
            auto rhs = parse_unary();
            if (!rhs)
                diag::error("Expected rhs for multiplicative expression");

            e = std::move(make_binary(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_additive() {
        auto e = parse_multiplicative();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "+", "-")) {
            fetch();
            auto rhs = parse_multiplicative();
            if (!rhs)
                diag::error("Expected rhs for additive expression");

            e = std::move(make_binary(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_relational() {
        auto e = parse_additive();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "<", "<=", ">", ">=")) {
            fetch();
            auto rhs = parse_additive();
            if (!rhs)
                diag::error("Expected rhs for comparison expression");

            e = std::move(make_relational(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_equality() {
        auto e = parse_relational();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "==", "!=")) {
            fetch();
            auto rhs = parse_relational();
            if (!rhs)
                diag::error("Expected rhs for equality expression");

            e = std::move(make_relational(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_assignment() {
        auto e = parse_or();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "=", "+=", "-=", "*=", "/=")) {
            fetch();
            auto rhs = parse_assignment();
            if (!rhs)
                diag::error(*t, "Expected rhs for assignment expression");

            auto *eid = std::get_if<ast::identifier_data>(&e->data);
            if (!eid)
                diag::error(*t, "Lhs of an assignment must be an identifier");

            if (t->data == "=") {
                ast::assign_data ad{};
                ad.id = *eid;
                ad.val = make_expr(std::move(rhs.value()));
                location loc = e->src_loc;
                e = expr{.src_loc = e->src_loc, .data = std::move(ad)};
            } else
                e = make_compound_assignment(std::move(e.value()), std::move(rhs.value()), t->data);
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_and() {
        auto e = parse_equality();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "&&")) {
            fetch();
            auto rhs = parse_equality();
            if (!rhs)
                diag::error(*t, "Expected rhs for assignment expression");

            e = std::move(make_binary(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_or() {
        auto e = parse_and();
        if (!e)
            return {};

        while (auto t = match_any(cat::punct, "||")) {
            fetch();
            auto rhs = parse_and();
            if (!rhs)
                diag::error(*t, "Expected rhs for assignment expression");

            e = std::move(make_binary(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_expr() {
        return parse_assignment();
    }

    std::optional<ast::expr> parser::parse_block() {
        if (!match(cat::punct, "{"))
            return {};

        auto loc = fetch().loc;
        ast::block_data bd{};

        while (!match(cat::punct, "}")) {
            if (empty())
                diag::error(peek(), "Expected '}' to close block");

            if (match_any(cat::keyword, "let", "return", "break", "continue")) {
                auto s = parse_stmt();
                if (!s)
                    diag::error(peek(), "Expected statement in block");
                bd.stmts.push_back(make_stmt(std::move(*s)));
                continue;
            }

            auto e = parse_expr();
            if (!e)
                diag::error(peek(), "Expected statement or expression in block");

            if (match(cat::punct, ";")) {
                fetch();
                bd.stmts.push_back(make_stmt(stmt{.src_loc = e->src_loc,
                    .data = ast::expr_stmt_data{.expr = make_expr(std::move(*e))}}));
                continue;
            }

            if (match(cat::punct, "}")) {
                bd.trailing = make_expr(std::move(*e));
                break;
            }

            if (may_omit_statement_semicolon(*e)) {
                bd.stmts.push_back(make_stmt(stmt{.src_loc = e->src_loc,
                    .data = ast::expr_stmt_data{.expr = make_expr(std::move(*e))}}));
                continue;
            }
            diag::error(peek(), "Expected ';' or '}' after expression in block");
        }

        fetch();
        return ast::expr{.src_loc = loc, .data = std::move(bd)};
    }

    std::optional<ast::expr> parser::parse_if_expr() {
        if (!match(cat::keyword, "if"))
            return {};

        auto t = fetch();
        parsing_control_condition = true;
        auto cond = parse_expr();
        parsing_control_condition = false;
        if (!cond)
            diag::error(t, "Expected condition after if");

        auto then_body = parse_block();
        if (!then_body)
            diag::error(t, "Expected block after if condition");

        ast::expr_ptr else_body = nullptr;
        if (match(cat::keyword, "else")) {
            fetch();

            if (match(cat::keyword, "if")) {
                auto else_if = parse_if_expr();
                if (!else_if)
                    diag::error(t, "Expected if expression in else if");
                else_body = make_expr(std::move(*else_if));
            } else {
                auto else_blk = parse_block();
                if (!else_blk)
                    diag::error(t, "Expected block after else");
                else_body = make_expr(std::move(*else_blk));
            }
        }

        return ast::expr{
            .src_loc = t.loc,
            .data = ast::if_data{
                .cond = make_expr(std::move(*cond)),
                .then_body = make_expr(std::move(*then_body)),
                .else_body = std::move(else_body)
            }
        };
    }

    std::optional<ast::expr> parser::parse_match_expr() {
        if (!match(cat::keyword, "match"))
            return {};

        auto t = fetch();
        parsing_control_condition = true;
        auto expr = parse_expr();
        parsing_control_condition = false;
        if (!expr)
            diag::error(t, "Expected expression after match");

        require(cat::punct, "{");
        std::vector<ast::match_arm> arms;

        if (match(cat::punct, "}"))
            diag::error(peek(), "Match expressions require at least one arm");

        {
            while (true) {
                auto pattern = parse_pattern();
                if (!pattern)
                    diag::error(peek(), "Expected pattern in match arm");

                require(cat::punct, "=>");
                auto arm_expr = parse_expr();
                if (!arm_expr)
                    diag::error(t, "Expected expression in match arm");

                arms.push_back(ast::match_arm{
                    .pattern = make_expr(std::move(*pattern)),
                    .expr = make_expr(std::move(*arm_expr))
                });

                if (!match(cat::punct, ",")) break;
                fetch();
                if (match(cat::punct, "}")) break;
            }
        }

        require(cat::punct, "}");
        return ast::expr{
            .src_loc = t.loc,
            .data = ast::match_data{
                .expr = make_expr(std::move(*expr)),
                .arms = std::move(arms)
            }
        };
    }

    std::optional<ast::expr> parser::parse_pattern() {
        if (match(cat::number) || match(cat::string) ||
            match_any(cat::keyword, "true", "false"))
            return parse_primary();

        if (!match(cat::ident))
            return {};

        auto name = fetch();
        if (match(cat::punct, "(")) {
            fetch();
            if (!match(cat::punct, ")")) {
                while (true) {
                    auto nested = parse_pattern();
                    if (!nested)
                        diag::error(peek(), "Expected nested pattern");
                    if (!match(cat::punct, ","))
                        break;
                    fetch();
                    if (match(cat::punct, ")"))
                        diag::error(peek(), "Trailing commas are not allowed in patterns");
                }
            }
            require(cat::punct, ")");
        }

        return ast::expr{.src_loc = name.loc,
            .data = ast::identifier_data{.id = name.data}};
    }

    std::optional<ast::expr> parser::parse_loop_expr() {
        if (!match(cat::keyword, "loop"))
            return {};

        auto t = fetch();
        auto body = parse_block();
        if (!body)
            diag::error(t, "Expected block after loop");

        return ast::expr{
            .src_loc = t.loc,
            .data = ast::loop_data{
                .body = make_expr(std::move(*body))
            }
        };
    }

    std::optional<ast::expr> parser::parse_while_expr() {
        if (!match(cat::keyword, "while"))
            return {};

        auto t = fetch();
        parsing_control_condition = true;
        auto cond = parse_expr();
        parsing_control_condition = false;
        if (!cond)
            diag::error(t, "Expected condition in while expression");

        auto body = parse_block();
        if (!body)
            diag::error(t, "Expected block after while condition");

        return ast::expr{
            .src_loc = t.loc,
            .data = ast::while_data{
                .cond = make_expr(std::move(*cond)),
                .body = make_expr(std::move(*body))
            }
        };
    }

    std::optional<ast::stmt> parser::parse_expr_stmt() {
        auto e = parse_expr();
        if (!e)
            return {};

        require(cat::punct, ";");
        return stmt{.src_loc = e->src_loc, .data = ast::expr_stmt_data{.expr = make_expr(std::move(*e))}};
    }

    std::optional<ast::stmt> parser::parse_ret() {
        if (!match(cat::keyword, "return"))
            return {};

        auto t = fetch();

        ast::ret_data rd{};
        if (match(cat::punct, ";")) {
            fetch();
            return stmt{.data = std::move(rd)};
        }

        auto e = parse_expr();
        if (!e)
            diag::error(t, "Expected expression");

        rd.val = make_expr(std::move(e.value()));
        require(cat::punct, ";");
        return stmt{.data = std::move(rd)};
    }

    std::optional<ast::stmt> parser::parse_control_stmt() {
        if (match(cat::keyword, "break")) {
            fetch();
            require(cat::punct, ";");
            return stmt{.data = ast::brk_data{}};
        }

        if (match(cat::keyword, "continue")) {
            fetch();
            require(cat::punct, ";");
            return stmt{.data = ast::cont_data{}};
        }

        return {};
    }

    std::optional<ast::stmt> parser::parse_let() {
        if (!match(cat::keyword, "let"))
            return {};

        auto loc = fetch().loc;
        var_decl vd{};

        if (match(cat::keyword, "mut")) {
            fetch();
            vd.mut_modifier = var_decl::mut_t::mut;
        } else {
            vd.mut_modifier = var_decl::mut_t::imut;
        }

        vd.name = require(cat::ident).data;

        if (match(cat::punct, ":")) {
            auto t = fetch();
            auto ty = parse_type_annotation();
            if (!ty)
                diag::error(t, "Invalid type annotation");
            vd.ty = std::move(ty);
        }

        auto t = require(cat::punct, "=");
        auto e = parse_expr();
        if (!e)
            diag::error(t, "Expected expression in let statement");
        vd.initializer = make_expr(std::move(e.value()));
        require(cat::punct, ";");

        return stmt{.src_loc = loc, .data = ast::let_data{.decl = std::move(vd)}};
    }


    std::optional<ast::type_annotation> parser::parse_type_annotation() {
        ast::type_annotation ty{};

        if (match(cat::punct, "&")) {
            fetch();
            ty.is_reference = true;
            if (match(cat::keyword, "mut")) {
                fetch();
                ty.is_mutable_ref = true;
            }
        }

        token t;
        if (match(cat::ident)) {
            t = fetch();
            ty.base_name = t.data;
        } else if (match_any(cat::keyword, "i8", "i16", "i32", "i64",
                             "u8", "u16", "u32", "u64", "bool", "unit")) {
            t = fetch();
            ty.base_name = t.data;
        } else {
            diag::error("Invalid type annotation");
        }

        if (match(cat::punct, "<")) {
            fetch();
            if (match(cat::punct, ">"))
                diag::error(peek(), "Generic argument lists require at least one type");
            while (!match(cat::punct, ">")) {
                auto generic_arg = parse_type_annotation();
                if (!generic_arg)
                    diag::error("Expected type argument");
                ty.generic_args.push_back(*generic_arg);

                if (!match(cat::punct, ","))
                    break;
                fetch();
            }
            require(cat::punct, ">");
        }

        if (match(cat::punct, "?")) {
            fetch();
            ty.is_nullable = true;
        }

        while (match(cat::punct, "[")) {
            fetch();
            require(cat::punct, "]");
            ty.is_array = true;
        }

        return ty;
    }

    std::optional<ast::param> parser::parse_param() {
        ast::param p{};
        p.name = require(cat::ident).data;
        require(cat::punct, ":");
        auto ty = parse_type_annotation();
        if (!ty)
            diag::error("Invalid type");
        p.ty = std::move(ty.value());
        return p;
    }

    std::optional<ast::param_list> parser::parse_param_list() {
        ast::param_list pl{};
        if (match(cat::punct, ")"))
            return pl;

        while (!match(cat::punct, ")")) {
            auto p = parse_param();
            if (!p)
                diag::error("Expected a parameter");
            pl.params.push_back(p.value());

            if (!match(cat::punct, ","))
                break;
            fetch();
            if (match(cat::punct, ")"))
                diag::error(peek(), "Trailing commas are not allowed in parameter lists");
        }

        return pl;
    }

    std::optional<ast::stmt> parser::parse_stmt() {
        std::optional<stmt> res{};
        if ((res = parse_ret())
            || (res = parse_control_stmt())
            || (res = parse_let())
            || (res = parse_expr_stmt()))
            return res;

        return {};
    }

    std::optional<ast::toplevel> parser::parse_global_var_decl() {
        if (!match(cat::keyword, "static"))
            return {};

        auto loc = fetch().loc;
        global_var gv{};

        if (match(cat::keyword, "mut")) {
            fetch();
            gv.is_mutable = true;
        } else {
            gv.is_mutable = false;
        }

        gv.name = require(cat::ident).data;
        require(cat::punct, ":");

        auto ty = parse_type_annotation();
        if (!ty)
            diag::error("Expected type annotation for global variable");
        gv.ty = *ty;

        require(cat::punct, "=");
        auto init = parse_expr();
        if (!init)
            diag::error("Expected initializer for global variable");
        gv.initializer = make_expr(std::move(*init));

        require(cat::punct, ";");
        return toplevel{.loc = loc, .data = std::move(gv)};
    }

    std::vector<std::string_view> parser::parse_generic_params() {
        std::vector<std::string_view> gp{};
        if (!match(cat::punct, "<"))
            return gp;

        fetch();
        auto id = require(cat::ident);
        gp.push_back(id.data);

        while (!match(cat::punct, ">")) {
            require(cat::punct, ",");
            auto tmp = require(cat::ident);
            gp.push_back(tmp.data);
        }

        require(cat::punct, ">");
        return gp;
    }

    std::optional<ast::toplevel> parser::parse_fn_decl() {
        if (!match(cat::keyword, "fn"))
            return {};

        auto loc = fetch().loc;
        fn_decl fd;
        fd.name = require(cat::ident).data;
        fd.generics = std::move(parse_generic_params());

        require(cat::punct, "(");
        fd.params = parse_param_list().value_or(ast::param_list{});
        require(cat::punct, ")");

        fd.ret_ty = std::nullopt;
        if (match(cat::punct, "->")) {
            fetch();
            auto ret_ty = parse_type_annotation();
            if (!ret_ty)
                diag::error("Invalid return type");
            fd.ret_ty = std::move(ret_ty);
        }

        auto body = parse_block();
        if (!body)
            diag::error("Expected block in function body");

        fd.body = make_expr(std::move(*body));
        return toplevel{.loc = loc, .data = std::move(fd)};
    }

    std::vector<ast::enum_member> parser::parse_members() {
        std::vector<ast::enum_member> res{};

        if (match(cat::punct, "}"))
            return res;

        while (true) {
            auto member_name = require(cat::ident).data;
            std::vector<ast::type_annotation> data_types;

            if (match(cat::punct, "(")) {
                fetch();
                while (!match(cat::punct, ")")) {
                    auto ty = parse_type_annotation();
                    if (!ty)
                        diag::error("Expected type in enum member");
                    data_types.push_back(*ty);

                    if (!match(cat::punct, ","))
                        break;
                    fetch();
                }
                require(cat::punct, ")");
            }

            res.push_back(ast::enum_member{
                .name = member_name,
                .data_types = std::move(data_types)
            });

            if (!match(cat::punct, ","))
                break;
            fetch();
            if (match(cat::punct, "}"))
                break;
        }

        return res;
    }

    std::optional<ast::toplevel> parser::parse_enum_decl() {
        if (!match(cat::keyword, "enum"))
            return {};

        auto loc = fetch().loc;
        enum_decl ed{};
        ed.name = require(cat::ident).data;
        ed.generics = std::move(parse_generic_params());

        require(cat::punct, "{");
        if (match(cat::punct, "}"))
            diag::error(peek(), "Enum declarations require at least one member");
        ed.members = std::move(parse_members());
        require(cat::punct, "}");

        return toplevel{.loc = loc, .data = std::move(ed)};
    }

    std::optional<ast::toplevel> parser::parse_struct_decl() {
        if (!match(cat::keyword, "struct"))
            return {};

        auto loc = fetch().loc;
        struct_decl sd{};
        sd.name = require(cat::ident).data;
        sd.generics = std::move(parse_generic_params());

        require(cat::punct, "{");

        if (!match(cat::punct, "}")) {
            while (true) {
                auto field_name = require(cat::ident).data;
                require(cat::punct, ":");
                auto field_ty = parse_type_annotation();
                if (!field_ty)
                    diag::error("Expected type in struct field");

                sd.fields.push_back(ast::struct_field{
                    .name = field_name,
                    .ty = *field_ty
                });

                if (!match(cat::punct, ","))
                    break;
                fetch();
                if (match(cat::punct, "}"))
                    break;
            }
        }

        require(cat::punct, "}");
        return toplevel{.loc = loc, .data = std::move(sd)};
    }

    std::optional<ast::toplevel> parser::parse_toplevel() {
        std::optional<toplevel> res{};
        if ((res = parse_fn_decl())
            || (res = parse_enum_decl())
            || (res = parse_struct_decl())
            || (res = parse_global_var_decl()))
            return res;

        return {};
    }

    std::optional<ast::module> parser::parse_module() {
        ast::module m{};
        while (!empty()) {
            auto t = peek();
            auto top = parse_toplevel();
            if (!top)
                diag::error(t, "Expected toplevel declaration");
            m.toplevel_items.push_back(std::move(top.value()));
        }
        return m;
    }
}
