#pragma once

#include <cassert>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "symbol_table.hpp"
#include "../frontend/ast.hpp"
#include "types.hpp"

namespace dungeon::sema {
    struct semantic_analyzer {
        analysis_result semantics;

        // TODO: add support for structures
        structure create_structure() {
            struct_id id{.value = static_cast<uint32_t>(semantics.structures.size())};
            return {.id = id};
        }

        // TODO: add support for enums
        enumeration create_enumeration() {
            enum_id id{.value = static_cast<uint32_t>(semantics.enumerations.size())};
            enumeration e = {.id = id};
            semantics.enumerations.push_back(e);
            return e;
        }

        function create_function(const std::vector<ast::param> &params, const ast::type_annotation &ret_annot) {
            fn_id id{.value = static_cast<uint32_t>(semantics.functions.size())};
            function fn = {.id = id};
            fn.return_type = type_from_annotation(ret_annot);
            for (auto &p: params) {
                fn.param_types.push_back(type_from_annotation(p.ty));
            }
            semantics.functions.push_back(fn);
            return fn;
        }

        scope create_scope(std::optional<scope_id> parent_scope, std::optional<fn_id> enclosing_fn,
                           scope::kind_t kind) {
            scope_id id{.value = static_cast<uint32_t>(semantics.scopes.size())};
            scope curr = {.id = id, .parent_scope = parent_scope, .enclosing_fn = enclosing_fn, .kind = kind};
            semantics.scopes.push_back(curr);
            return curr;
        }

        symbol create_symbol(name_id nid, const src_location &src_loc, symbol::data_t data) {
            symbol_id sid{.value = static_cast<uint32_t>(semantics.symbols.size())};
            symbol sym = {.id = sid, .nid = nid, .src_loc = src_loc, .data = std::move(data)};
            semantics.symbols.push_back(sym);
            return sym;
        }

        enumeration &get_enumeration(enum_id id) { return semantics.enumerations.at(id.value); }
        function &get_function(fn_id fid) { return semantics.functions.at(fid.value); }
        scope &get_scope(scope_id sid) { return semantics.scopes.at(sid.value); }
        symbol &get_symbol(symbol_id sym_id) { return semantics.symbols.at(sym_id.value); }

        static variable::storage_t convert_storage(ast::var_decl::stor_t stor) {
            if (stor == ast::var_decl::global)
                return variable::global;
            if (stor == ast::var_decl::local)
                return variable::local;
            assert(false && "Unknown storage type");
        }

        static variable::mod_t convert_modifier(ast::var_decl::mod_t mod) {
            if (mod == ast::var_decl::mut)
                return variable::mod_t::mut;
            if (mod == ast::var_decl::imut)
                return variable::mod_t::imut;
            assert(false && "Invalid variable modifier!");
        }

        std::pair<bool, name_id> intern_name(std::string_view name) {
            std::string sname = std::string(name);
            if (auto it = semantics.interned_names.find(sname); it != semantics.interned_names.end()) {
                return {false, it->second};
            }

            name_id nid{.value = static_cast<uint32_t>(semantics.names.size())};
            semantics.names.push_back(sname);
            semantics.interned_names[sname] = nid;
            return {true, nid};
        }

        symbol_id declare(std::string_view name, const src_location &src_loc, symbol::data_t data, scope_id sid) {
            const auto &[_, nid] = intern_name(name);
            scope &scope = get_scope(sid);
            symbol sym = create_symbol(nid, src_loc, std::move(data));

            // TODO: do we want this ? Rust doesn't do it like this ....
            if (scope.symbols.contains(nid))
                diag::error("Symbol already present", sym.src_loc);

            scope.symbols[nid] = sym.id;
            return sym.id;
        }

        std::optional<symbol_id> lookup_symbol(std::string_view name, scope_id sid) {
            const auto &[inserted, nid] = intern_name(name);
            if (inserted)
                return {};

            std::optional curr_id = sid;
            while (curr_id) {
                scope &curr = get_scope(*curr_id);
                auto it = curr.symbols.find(nid);
                if (it != curr.symbols.end())
                    return it->second;
                curr_id = curr.parent_scope;
            }
            return {};
        }

        bool is_scope_inside_loop(scope_id curr_id) const {
            while (semantics.scopes.at(curr_id.value).kind != scope::loop) {
                auto next_id = semantics.scopes.at(curr_id.value).parent_scope;
                if (!next_id)
                    return false;
                curr_id = next_id.value();
            }
            return true;
        }

        const type *type_from_annotation(std::optional<ast::type_annotation> opt_annot) {
            if (!opt_annot)
                return nullptr;

            const auto &annotation = *opt_annot;
            if (annotation.base_name == "i8") {
                return semantics.types.get_int(8);
            }
            if (annotation.base_name == "i16") {
                return semantics.types.get_int(16);
            }
            if (annotation.base_name == "i32") {
                return semantics.types.get_int(32);
            }
            if (annotation.base_name == "i64") {
                return semantics.types.get_int(64);
            }
            if (annotation.base_name == "u8") {
                return semantics.types.get_uint(8);
            }
            if (annotation.base_name == "u16") {
                return semantics.types.get_uint(16);
            }
            if (annotation.base_name == "u32") {
                return semantics.types.get_uint(32);
            }
            if (annotation.base_name == "u64") {
                return semantics.types.get_uint(64);
            }
            if (annotation.base_name == "bool") {
                return semantics.types.get_bool();
            }
            if (annotation.base_name == "unit") {
                return semantics.types.get_unit();
            }
            assert(false && "unknown type");
        }

        const symbol &require_symbol(std::string_view name, const src_location &loc, scope_id sid) {
            auto sym_id = lookup_symbol(name, sid);
            if (!sym_id)
                diag::error("Unknown identifier", loc);
            return get_symbol(*sym_id);
        }

        const type *record(ast::expr &expr, const type *ty) {
            semantics.expr_ty[&expr] = ty;
            return ty;
        }

        const type *check(ast::expr &expr, const type *expected, scope_id sid) {
            const scope &curr_scope = get_scope(sid);

            if (std::get_if<ast::num_lit_data>(&expr.data)) {
                if (!is_integer(expected))
                    diag::error("Actual type doesn't match the expected" , expr.src_loc);
                return record(expr, expected);
            }
            if (auto ud = std::get_if<ast::unary_data>(&expr.data)) {
                check(*ud->lhs, expected, sid);
                infer_unary(ud->op, expected, semantics.types);
                return record(expr, expected);
            }
            if (auto bd = std::get_if<ast::binary_data>(&expr.data)) {
                check(*bd->lhs, expected, sid);
                check(*bd->rhs, expected, sid);
                return record(expr, expected);
            }
            if (auto ifd = std::get_if<ast::if_data>(&expr.data)) {
                auto cond = infer(*ifd->cond, sid);
                if (!is_boolean(cond))
                    diag::error("Expected a condition", expr.src_loc);

                scope then_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                check(*ifd->then_body, expected, then_scope.id);

                if (ifd->else_body) {
                    scope else_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                    check(*ifd->else_body, expected, else_scope.id);
                } else if (!compatible_types(expected, semantics.types.get_unit())) {
                    diag::error("if without else must be unit-typed", expr.src_loc);
                }
                return record(expr, expected);
            }
            if (auto blk = std::get_if<ast::block_data>(&expr.data)) {
                scope block_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                for (auto &s: blk->stmts)
                    if (auto ty = analyze(*s, block_scope.id); !is_unit(ty))
                        diag::error("Expected a unit type", s->src_loc);

                if (blk->trailing) {
                    check(*blk->trailing, expected, block_scope.id);
                } else if (!compatible_types(expected, semantics.types.get_unit())) {
                    diag::error("Expected a non-unit block to have a value", expr.src_loc);
                }
                return record(expr, expected);
            }

            const type *res = infer(expr, sid);
            if (!compatible_types(res, expected))
                diag::error("Type mismatch: expected does not match inferred type", expr.src_loc);
            return res;
        }

        const type *infer(ast::expr &expr, scope_id sid) {
            const scope &curr_scope = get_scope(sid);
            if (std::get_if<ast::num_lit_data>(&expr.data)) {
                return record(expr, semantics.types.get_int(32));
            }
            if (std::get_if<ast::bool_lit_data>(&expr.data)) {
                return record(expr, semantics.types.get_bool());
            }
            if (auto id = std::get_if<ast::identifier_data>(&expr.data)) {
                const auto &sym = require_symbol(id->name, expr.src_loc, sid);
                auto var = std::get_if<variable>(&sym.data);
                if (!var)
                    diag::error("Expected a variable");
                semantics.id_symbols[&expr] = sym.id;
                return record(expr, var->ty);
            }
            if (auto ud = std::get_if<ast::unary_data>(&expr.data)) {
                auto ty = infer(*ud->lhs, sid);
                return record(expr, infer_unary(ud->op, ty, semantics.types));
            }
            if (auto bd = std::get_if<ast::binary_data>(&expr.data)) {
                auto lhs = infer(*bd->lhs, sid);
                auto rhs = infer(*bd->rhs, sid);
                return record(expr, infer_binary(bd->op, lhs, rhs, semantics.types));
            }
            if (auto rd = std::get_if<ast::relational_data>(&expr.data)) {
                auto lhs = infer(*rd->lhs, sid);
                auto rhs = infer(*rd->rhs, sid);
                return record(expr, infer_relational(rd->op, lhs, rhs, semantics.types));
            }
            if (auto ad = std::get_if<ast::assign_data>(&expr.data)) {
                const auto &sym = require_symbol(ad->id.name, expr.src_loc, sid);
                auto var = std::get_if<variable>(&sym.data);
                if (!var)
                    diag::error("Unexpected lhs of an assignment", expr.src_loc);

                if (var->modifier != variable::mut)
                    diag::error("Cannot assign to a non-mutable variable", expr.src_loc);

                check(*ad->val, var->ty, sid);
                semantics.id_symbols[&expr] = sym.id;
                return record(expr, var->ty);
            }
            if (auto cd = std::get_if<ast::call_data>(&expr.data)) {
                // TODO: in the future add support for overloads
                auto id = std::get_if<ast::identifier_data>(&cd->callee->data);
                if (!id)
                    diag::error("Not implemented yet");

                const symbol &sym = require_symbol(id->name, expr.src_loc, sid);
                auto fn = std::get_if<function>(&sym.data);
                if (!fn)
                    diag::error("Expected a function", expr.src_loc);

                semantics.expr_fn[&expr] = fn;
                if (cd->args.size() != fn->param_types.size())
                    diag::error("Mismatch in the amount of arguments", expr.src_loc);

                for (size_t i = 0; i < fn->param_types.size(); ++i)
                    check(*cd->args[i], fn->param_types[i], sid);

                return record(expr, fn->return_type);
            }
            if (auto fad = std::get_if<ast::field_access_data>(&expr.data)) {
                return nullptr; // TODO
            }
            if (auto aid = std::get_if<ast::array_index_data>(&expr.data)) {
                return nullptr; // TODO
            }
            if (auto ifd = std::get_if<ast::if_data>(&expr.data)) {
                auto cond = infer(*ifd->cond, sid);
                if (!is_boolean(cond)) {
                    diag::error("Expected a condition", expr.src_loc);
                }

                scope then_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                auto then_ty = infer(*ifd->then_body, then_scope.id);

                if (ifd->else_body) {
                    scope else_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                    auto else_ty = infer(*ifd->else_body, else_scope.id);
                    if (!compatible_types(then_ty, else_ty))
                        diag::error("Incompatible semantics.types in branches", expr.src_loc);
                } else if (!is_unit(then_ty)) {
                    diag::error("If without else must be unit-typed", expr.src_loc);
                }
                return record(expr, then_ty);
            }
            if (auto wd = std::get_if<ast::while_data>(&expr.data)) {
                auto cond = infer(*wd->cond, sid);
                if (!is_boolean(cond)) {
                    diag::error("Expected a condition", expr.src_loc);
                }

                scope while_scope = create_scope(sid, curr_scope.enclosing_fn, scope::loop);
                infer(*wd->body, while_scope.id);
                return record(expr, semantics.types.get_unit());
            }
            if (auto ld = std::get_if<ast::loop_data>(&expr.data)) {
                scope loop_scope = create_scope(sid, curr_scope.enclosing_fn, scope::loop);
                infer(*ld->body, loop_scope.id);
                return record(expr, semantics.types.get_unit());
            }
            if (auto md = std::get_if<ast::match_data>(&expr.data)) {
                return nullptr;
            }
            if (auto sd = std::get_if<ast::struct_literal_data>(&expr.data)) {
                return nullptr;
            }
            if (auto blk = std::get_if<ast::block_data>(&expr.data)) {
                scope block_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                for (auto &s: blk->stmts) {
                    if (auto ty = analyze(*s, block_scope.id); !is_unit(ty))
                        diag::error("Expected a unit type", s->src_loc);
                }
                if (blk->trailing)
                    return record(expr, infer(*blk->trailing, block_scope.id));
                return record(expr, semantics.types.get_unit());
            }

            assert(false && "Non-exhaustive data cases!");
        }

        const type *analyze(const ast::var_decl &vdecl, const src_location &loc, scope_id sid) {
            // TODO: I guess we do not need to require an initilizer, but we should check that we aren't using unitialized variables
            assert(vdecl.initializer && "Expected an initializer");
            const type *var_ty = vdecl.ty
                                     ? check(*vdecl.initializer, type_from_annotation(vdecl.ty), sid)
                                     : infer(*vdecl.initializer, sid);

            variable v{
                .modifier = convert_modifier(vdecl.modifier), .storage = convert_storage(vdecl.storage),
                .ty = var_ty,
            };

            semantics.var_decl_symbols[&vdecl] = declare(vdecl.name, loc, v, sid);
            return var_ty;
        }

        const type *analyze(const ast::stmt &stmt, scope_id sid) {
            const scope &curr_scope = get_scope(sid);

            if (auto ld = std::get_if<ast::let_data>(&stmt.data)) {
                analyze(ld->decl, stmt.src_loc, sid);
                return semantics.types.get_unit();
            }
            if (auto exp = std::get_if<ast::expr_stmt_data>(&stmt.data)) {
                infer(*exp->expr, sid);
                return semantics.types.get_unit();
            }
            if (auto rd = std::get_if<ast::ret_data>(&stmt.data)) {
                if (!curr_scope.enclosing_fn)
                    diag::error("Return statement used outside of a function body");
                auto &enclosing_fn = get_function(*curr_scope.enclosing_fn);
                if (rd->val)
                    check(*rd->val, enclosing_fn.return_type, sid);
                else if (!is_unit(enclosing_fn.return_type))
                    diag::error("Missing return value", stmt.src_loc);
                return semantics.types.get_unit();
            }
            if (std::get_if<ast::brk_data>(&stmt.data)) {
                if (!is_scope_inside_loop(sid))
                    diag::error("'break' used outside of a loop");
                return semantics.types.get_unit();
            }
            if (std::get_if<ast::cont_data>(&stmt.data)) {
                if (!is_scope_inside_loop(sid))
                    diag::error("'continue' used outside of a loop");
                return semantics.types.get_unit();
            }

            assert(false && "Non-exhaustive data cases!");
        }

        bool has_trailing_expr(const ast::fn_decl &fd) {
            if (!fd.body)
                return false;
            auto t = std::get_if<ast::block_data>(&fd.body->data);
            if (!t)
                return false;

            return t->trailing != nullptr;
        }

        void analyze(const ast::module &module, scope_id global_id) {
            for (const auto &[loc, data]: module.toplevel_items) {
                if (const auto fd = std::get_if<ast::fn_decl>(&data)) {
                    const auto &sym = require_symbol(fd->name, loc, global_id);
                    auto fn = std::get_if<function>(&sym.data);
                    if (!fn)
                        diag::error("The symbol ", fd->name, " does not correspond to a function", loc);

                    scope fn_scope = create_scope(global_id, fn->id, scope::function);
                    for (auto &param: fd->params.params) {
                        variable v{
                            .modifier = variable::imut, .storage = variable::local, .ty = type_from_annotation(param.ty)
                        };
                        declare(param.name, loc, v, fn_scope.id);
                    }

                    check(*fd->body, fn->return_type, fn_scope.id);
                } else if (const auto ed = std::get_if<ast::enum_decl>(&data)) {
                    // TODO:
                } else if (const auto sd = std::get_if<ast::struct_decl>(&data)) {
                    // TODO
                } else if (const auto gvd = std::get_if<ast::global_var_decl>(&data)) {
                    analyze(gvd->decl, loc, global_id);
                } else {
                    assert(false && "Non-exhaustive data cases!");
                }
            }
        }

        void collect_toplevel_declarations(const ast::module &module, scope_id global_id) {
            for (const auto &[loc, data]: module.toplevel_items) {
                if (const auto fd = std::get_if<ast::fn_decl>(&data)) {
                    // TODO: we should probably use the function signature as the key in the symtab, if we want to allow overloading
                    function f = create_function(fd->params.params, fd->ret_ty);
                    declare(fd->name, loc, std::move(f), global_id);
                } else if (const auto ed = std::get_if<ast::enum_decl>(&data)) {
                    enumeration e = create_enumeration();
                    declare(ed->name, loc, std::move(e), global_id);
                } else if (const auto sd = std::get_if<ast::struct_decl>(&data)) {
                    structure s = create_structure();
                    declare(sd->name, loc, std::move(s), global_id);
                }
            }
        }

        analysis_result run(const ast::module &module) {
            scope global = create_scope(std::nullopt, std::nullopt, scope::global);
            collect_toplevel_declarations(module, global.id);
            analyze(module, global.id);
            return semantics;
        }
    };
}
