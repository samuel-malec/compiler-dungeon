#include "parser.hpp"

namespace dungeon {
    std::optional<ast::expr> parser::parse_primary() {
        if (match(cat::punct, "(")) {
            fetch();
            auto e = parse_expr();
            require(cat::punct, ")");
            return e;
        }

        if (match(cat::number)) {
            auto tok = fetch();
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
            return ast::expr{.src_loc = tok.loc, .data = ast::identifier_data{.id = tok.data}};
        }

        return {};
    }

    std::optional<ast::expr> parser::parse_postfix() {
        auto e = parse_primary();
        if (!e)
            return {};

        while (match(cat::punct, "(")) {
            location loc = e->src_loc;
            fetch();

            ast::call_data cd{};
            auto *eid = std::get_if<ast::identifier_data>(&e->data);
            if (!eid)
                diag::error("Expected an identifier as a callee");

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
            e = expr{.src_loc = loc, .data = std::move(cd)};
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_unary() {
        if (auto t = match_any(cat::punct, "!", "-", "+")) {
            fetch();
            auto rhs = parse_unary();
            if (!rhs)
                diag::error("Expected unary operand");

            ast::unary_data ud{};
            ud.lhs = make_expr(std::move(rhs.value()));
            ud.op = op_kind_from_str(t->data);
            return expr{.src_loc = t->loc, .data = std::move(ud)};
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
                diag::error("Expected rhs for assignment expression");

            auto *eid = std::get_if<ast::identifier_data>(&e->data);
            if (!eid)
                diag::error("Lhs of an assignment must be an identifier");

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
                diag::error("Expected rhs for assignment expression");

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
                diag::error("Expected rhs for assignment expression");

            e = std::move(make_binary(std::move(e.value()), std::move(rhs.value()), op_kind_from_str(t->data)));
        }

        return e;
    }

    std::optional<ast::expr> parser::parse_expr() {
        return parse_assignment();
    }

    std::optional<ast::stmt> parser::parse_block() {
        if (!match(cat::punct, "{"))
            return {};

        fetch();
        ast::block_data bd{};

        while (!match(cat::punct, "}")) {
            auto s = parse_stmt();
            if (!s)
                diag::error("Parsing statement inside a block: ");
            bd.stmts.push_back(make_stmt(std::move(*s)));
        }

        fetch();
        return stmt{.data = std::move(bd)};
    }

    std::optional<ast::stmt> parser::parse_if() {
        if (!match(cat::keyword, "if"))
            return {};

        auto t = fetch();
        require(cat::punct, "(");

        auto cond = parse_expr();
        if (!cond)
            diag::error("Expected condition after if");

        require(cat::punct, ")");
        auto then_body = parse_stmt();
        if (!then_body)
            diag::error("Empty body inside if block");

        ast::if_data ifd{};
        ifd.cond = make_expr(std::move(cond.value()));
        ifd.then_body = make_stmt(std::move(then_body).value());
        if (!match(cat::keyword, "else"))
            return stmt{.src_loc = t.loc, .data = std::move(ifd)};

        fetch();
        auto else_body = parse_stmt();
        if (!else_body)
            diag::error("Empty else body");

        ifd.else_body = make_stmt(std::move(else_body.value()));
        return stmt{.src_loc = t.loc, .data = std::move(ifd)};
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
            diag::error("Expected expression");

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

    std::optional<parser::stmt> parser::parse_let() {
        auto vd = parse_var_decl_data();
        if (!vd)
            diag::error("Invalid variable declaration");
        return stmt{.data = ast::let_data{.decl = std::move(vd.value())}};
    }

    std::optional<ast::stmt> parser::parse_for() {
        if (!match(cat::keyword, "for"))
            return {};

        auto t = fetch();
        location loc = t.loc;

        require(cat::punct, "(");
        ast::for_data fd{};
        fd.init = nullptr;
        if (!match(cat::punct, ";")) {
            if (auto v = parse_let())
                fd.init = make_stmt(std::move(v.value()));
            else {
                // todo: what to actually consider as an initializer ? Allowing only assignment seems rather too restrictive...
                auto ie = parse_expr();
                if (!ie)
                    diag::error("Invalid for loop initializer");

                stmt tmp{.data = ast::expr_stmt_data{.expr = make_expr(std::move(ie.value()))}};
                fd.init = make_stmt(std::move(tmp));
                require(cat::punct, ";");
            }
        } else
            fetch();

        fd.cond = nullptr;
        if (!match(cat::punct, ";")) {
            auto ce = parse_expr();
            if (!ce)
                diag::error("Invalid for loop condition");
            fd.cond = make_expr(std::move(ce.value()));
        }

        require(cat::punct, ";");
        fd.update = nullptr;
        if (!match(cat::punct, ")")) {
            auto ue = parse_expr();
            if (!ue)
                diag::error("Invalid for loop update");
            fd.update = make_expr(std::move(ue.value()));
        }

        require(cat::punct, ")");
        auto body = parse_stmt();
        if (!body)
            diag::error("Empty body in for loop");

        fd.body = make_stmt(std::move(body).value());
        stmt res{.src_loc = loc, .data = std::move(fd)};
        return res;
    }

    std::optional<ast::stmt> parser::parse_while() {
        if (!match(cat::keyword, "while"))
            return {};
        fetch();

        require(cat::punct, "(");
        auto e = parse_expr();
        if (!e)
            diag::error("Empty condition in while");

        require(cat::punct, ")");
        auto body = parse_stmt();
        if (!body)
            diag::error("Empty body in while");

        ast::while_data wd{};
        wd.cond = make_expr(std::move(e.value()));
        wd.body = make_stmt(std::move(body).value());
        stmt res{.data = std::move(wd)};
        return res;
    }

    std::optional<ast::stmt> parser::parse_do_while() {
        if (!match(cat::keyword, "do"))
            return {};

        fetch();
        auto s = parse_stmt();
        if (!s)
            diag::error("Empty body in do_while");

        require(cat::keyword, "while");
        require(cat::punct, "(");
        auto e = parse_expr();
        if (!e)
            diag::error("Empty condition in do_while");

        require(cat::punct, ")");
        require(cat::punct, ";");

        ast::do_while_data dw{};
        dw.cond = make_expr(std::move(e.value()));
        dw.body = make_stmt(std::move(s.value()));
        stmt res{.data = std::move(dw)};
        return res;
    }

    std::optional<ast::stmt> parser::parse_loop_stmt() {
        std::optional<stmt> res{};
        if ((res = parse_for())
            || (res = parse_while())
            || (res = parse_do_while()))
            return res;

        return {};
    }

    std::optional<ast::var_decl> parser::parse_var_decl_data() {
        return std::nullopt;
    }

    std::optional<ast::type_annotation> parser::parse_type_annotation() {
        auto t = require(cat::ident);
        return ast::type_annotation{.base_name = t.data};
    }

    std::optional<ast::param> parser::parse_param() {
        ast::param p{};
        p.name = require(cat::ident).data;
        require(cat::punct, ";");
        auto ty = parse_type_annotation();
        if (!ty)
            diag::error("Invalid type");
        p.ty = std::move(ty.value());
        return p;
    }

    std::optional<ast::param_list> parser::parse_param_list() {
        ast::param_list pl{};
        while (!match(cat::punct, ")")) {
            auto p = parse_param();
            if (!p)
                diag::error("Expected a parameter");
            pl.params.push_back(p.value());
        }

        return pl;
    }

    std::optional<ast::stmt> parser::parse_stmt() {
        std::optional<stmt> res{};
        if ((res = parse_loop_stmt())
            || (res = parse_block())
            || (res = parse_if())
            || (res = parse_ret())
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
        auto vd = parse_var_decl_data();
        if (!vd)
            diag::error("Invalid variable declaration");

        global_var gv{.decl = std::move(vd.value())};
        return toplevel{.loc = loc, .data = std::move(gv)};
    }

    std::vector<std::string_view> parser::parse_generic_params() {
        std::vector<std::string_view> gp{};
        if (!match(cat::punct, "<"))
            return gp;

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
        fd.params = parse_param_list().value();
        require(cat::punct, ")");

        require(cat::punct, "->");
        auto ret_ty = parse_type_annotation();
        if (!ret_ty)
            diag::error("Invalid type");

        fd.ret_ty = std::move(ret_ty.value());
        fd.body = make_stmt(std::move(parse_stmt().value()));
        return toplevel{.loc = loc, .data = std::move(fd)};
    }

    std::vector<ast::enum_member> parser::parse_members() {
        std::vector<ast::enum_member> res{};

        return res;
    }


    std::optional<ast::toplevel> parser::parse_enum_decl() {
        // TODO
        if (!match(cat::punct, "enum"))
            return {};

        enum_decl ed{};
        ed.name = require(cat::punct).data;
        ed.members = std::move(parse_members());
        return {};
    }

    std::optional<ast::toplevel> parser::parse_struct_decl() {
        // TODO
        if (!match(cat::keyword, "struct"))
            return {};
        auto name = require(cat::ident);
        require(cat::punct, "{");
        return {};
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
            auto top = parse_toplevel();
            if (!top)
                diag::error("Expected toplevel declaration");
            m.toplevel_items.push_back(std::move(top.value()));
        }
        return m;
    }
}
