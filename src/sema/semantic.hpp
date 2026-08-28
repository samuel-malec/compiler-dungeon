#pragma once

#include <cassert>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../frontend/ast.hpp"
#include "types.hpp"

// TODO: what programs are valid and what are not ?
namespace dungeon {
    struct name_id {
        uint32_t value;
        bool operator<(const name_id &rhs) const { return value < rhs.value; }
    };

    struct symbol_id {
        uint32_t value;
    };

    struct fn_id {
        uint32_t value;
    };

    struct enum_id {
        uint32_t value;
    };

    struct struct_id {
        uint32_t value;
    };

    struct scope_id {
        uint32_t value;
    };

    // TODO: we will probably have to create a notion of a 'binding' instead...
    struct scope {
        scope_id id;
        std::optional<scope_id> parent_scope;
        std::optional<fn_id> enclosing_fn;

        enum kind_t { global, function, loop, block } kind;

        std::map<name_id, symbol_id> symbols;
    };

    struct function {
        fn_id id;
        std::vector<type *> param_types;
        type *return_type;
    };

    struct enumeration {
        enum_id id;
        // TODO
    };

    struct structure {
        struct_id id;
        // TODO;
    };

    struct variable {
        enum mod_t { mut, imut } modifier;

        enum storage_t { global, local } storage;
    };

    struct symbol {
        symbol_id id;
        name_id nid;
        src_location src_loc;
        using data_t = std::variant<std::monostate, function, structure, enumeration, variable>;
        data_t data;
    };

    // TODO: think about naming this analysis_context and adding typemanager here
    struct analysis_result {
        std::vector<std::string> names;
        std::vector<symbol> symbols;
        std::vector<enumeration> enumerations;
        std::vector<structure> structures;
        std::vector<function> functions;
        std::vector<scope> scopes;

        std::unordered_map<ast::expr *, scope_id> expr_scopes;
        std::unordered_map<ast::stmt *, scope_id> stmt_scopes;
        std::unordered_map<ast::var_decl *, scope_id> decl_scopes;

        std::unordered_map<ast::expr *, type> expr_types;
    };

    struct semantic_analyzer {
        analysis_result semantics;
        std::map<std::string, name_id, std::less<> > interned_names;

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
            if (auto it = interned_names.find(sname); it != interned_names.end()) {
                return {false, it->second};
            }

            name_id nid{.value = static_cast<uint32_t>(semantics.names.size())};
            semantics.names.push_back(sname);
            interned_names[sname] = nid;
            return {true, nid};
        }

        void declare(std::string_view name, const src_location &src_loc, symbol::data_t data, scope_id sid) {
            const auto &[_, nid] = intern_name(name);
            scope &scope = get_scope(sid);
            symbol sym = create_symbol(nid, src_loc, std::move(data));

            // TODO: do we want this ? Rust doesn't do it like this ....
            if (scope.symbols.contains(nid))
                diag::error("Symbol already present", sym.src_loc);

            scope.symbols[nid] = sym.id;
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


        // TODO: we want to implement the actualy type checking/inference
        type *analyze(ast::expr &expr, const scope_id sid) {
            const scope &curr_scope = get_scope(sid);
            if (auto id = std::get_if<ast::identifier_data>(&expr.data)) {
                auto sym = lookup_symbol(id->name, sid);
                if (!sym)
                    diag::error("Unknown identifier", expr.src_loc);
            } else if (auto ud = std::get_if<ast::unary_data>(&expr.data)) {
                // TODO: we actually don't want this, because this should be handled by parser,
                // but more like check if the unary op is compatible with the type ( e.g we can't use not with numerical types )
                if (!is_unary_op(ud->op))
                    diag::error("Not an unary expression");
                analyze(*ud->lhs, sid);
            } else if (auto bd = std::get_if<ast::binary_data>(&expr.data)) {
                analyze(*bd->lhs, sid);
                analyze(*bd->rhs, sid);
            } else if (auto rd = std::get_if<ast::relational_data>(&expr.data)) {
                analyze(*bd->lhs, sid);
                analyze(*bd->rhs, sid);
            } else if (auto ad = std::get_if<ast::assign_data>(&expr.data)) {
                auto sym_id= lookup_symbol(ad->id.name, sid);
                if ( !sym_id )
                    diag::error("Unknown identifier", expr.src_loc);

                const auto& sym = get_symbol(sym_id.value());
                auto var = std::get_if<variable>(&sym.data);
                if (!var)
                    diag::error("Unexpected lhs of an assignment", expr.src_loc);

                if (var->modifier != variable::mut )
                    diag::error("Cannot assign to a non-mutable variable");

                type *rhs = analyze(*ad->val, sid);

                // TODO: check compatible types

            } else if (auto cd = std::get_if<ast::call_data>(&expr.data)) {
            } else if (auto fad = std::get_if<ast::field_access_data>(&expr.data)) {
            } else if (auto aid = std::get_if<ast::array_index_data>(&expr.data)) {
            } else if (auto ifd = std::get_if<ast::if_data>(&expr.data)) {
                analyze(*ifd->cond, sid);
                scope then_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                analyze(*ifd->then_body, then_scope.id);
                if (ifd->else_body) {
                    scope else_scope = create_scope(sid, curr_scope.enclosing_fn, scope::block);
                    analyze(*ifd->else_body, else_scope.id);
                }
            } else if (auto wd = std::get_if<ast::while_data>(&expr.data)) {
                analyze(*wd->cond, sid);
                scope while_scope = create_scope(sid, curr_scope.enclosing_fn, scope::loop);
                analyze(*wd->body, while_scope.id);
            } else if (auto ld = std::get_if<ast::loop_data>(&expr.data)) {
                scope loop_scope = create_scope(sid, curr_scope.enclosing_fn, scope::loop);
                analyze(*ld->body, loop_scope.id);
            } else if (auto md = std::get_if<ast::match_data>(&expr.data)) {
            } else if (auto sd = std::get_if<ast::struct_literal_data>(&expr.data)) {
            } else if (auto blk = std::get_if<ast::block_data>(&expr.data)) {
                scope block_scope = create_scope( sid, curr_scope.enclosing_fn, scope::block);
                for ( auto& s : blk->stmts ) {

                }
            } else
                assert(false && "Non-exhaustive data cases!");

            return nullptr;
        }

        type *analyze(const ast::var_decl &vdecl, const src_location& loc, const scope_id sid) {
            assert( vdecl.initializer && "Expected an initializer");
            type* rhs = analyze( *vdecl.initializer, sid );

            variable v{
                .modifier = convert_modifier(vdecl.modifier), .storage = convert_storage(vdecl.storage)
            };
            declare(vdecl.name, loc, v, sid);

            // type* lhs = type_from_annotation(vdecl.ty);
            return nullptr;
        }

        type *analyze(const ast::stmt &stmt, const scope_id sid) {
            if (auto ld = std::get_if<ast::let_data>(&stmt.data)) {
                analyze(ld->decl, stmt.src_loc, sid);

            } else if (auto exp = std::get_if<ast::expr_stmt_data>(&stmt.data)) {
                analyze(*exp->expr, sid);
            } else if (auto rd = std::get_if<ast::ret_data>(&stmt.data)) {
                if (rd->val)
                    analyze(*rd->val, sid);
            } else if (auto bd = std::get_if<ast::brk_data>(&stmt.data)) {
                if (!is_scope_inside_loop(sid))
                    diag::error("'break' used outside of a loop");
            } else if (auto cd = std::get_if<ast::cont_data>(&stmt.data)) {
                if (!is_scope_inside_loop(sid))
                    diag::error("'continue' used outside of a loop");
            } else {
                assert(false && "Non-exhaustive data cases!");
            }

            return nullptr;
        }

        type *analyze(const ast::module &module, scope_id global_id) {
            for (const auto &[loc, data]: module.toplevel_items) {
                if (const auto fd = std::get_if<ast::fn_decl>(&data)) {
                    auto sym_id = lookup_symbol(fd->name, global_id);
                    if (!sym_id)
                        diag::error("Symbol lookup failed at: ", loc, " in function: ", fd->name);
                    const auto &sym = get_symbol(sym_id.value());

                    auto fn = std::get_if<function>(&sym.data);
                    if (!fn) {
                        diag::error("The symbol ", fd->name, " does not correspond to a function", loc);
                    }

                    scope fn_scope = create_scope(global_id, std::nullopt, scope::function);
                    for (auto &param: fd->param_list.params) {
                        variable v{.modifier = variable::imut, .storage = variable::local};
                        declare(param.name, loc, v, fn_scope.id);
                    }

                    type *body_ty = analyze(*fd->body, fn_scope.id);

                    // assert(compatible_types( body_ty, fn->return_type ));
                } else if (const auto ed = std::get_if<ast::enum_decl>(&data)) {
                    // TODO:
                } else if (const auto sd = std::get_if<ast::struct_decl>(&data)) {
                    // TODO
                } else if (const auto gvd = std::get_if<ast::global_var_decl>(&data)) {
                    // FIXME: I guess we can hardcode global storage here ?
                    variable v{
                        .modifier = convert_modifier(gvd->decl.modifier), .storage = convert_storage(gvd->decl.storage)
                    };
                    declare(gvd->decl.name, loc, v, global_id);
                } else {
                    assert(false && "Non-exhaustive data cases!");
                }
            }
            return nullptr;
        }

        void collect_toplevel_declarations(const ast::module &module, scope_id global_id) {
            for (const auto &[loc, data]: module.toplevel_items) {
                if (const auto fd = std::get_if<ast::fn_decl>(&data)) {
                    function f = create_function(fd->param_list.params, fd->ret_ty);
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
            std::cout << "Running semantic analysis\n";
            scope global = create_scope(std::nullopt, std::nullopt, scope::global);
            collect_toplevel_declarations(module, global.id);
            analyze(module, global.id);
            return semantics;
        }
    };
}
