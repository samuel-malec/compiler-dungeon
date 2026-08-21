#pragma once

#include <cassert>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "../frontend/ast.hpp"

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

    struct scope {
        scope_id id;
        std::optional<scope_id> parent_scope;
        std::optional<fn_id> enclosing_fn;
        enum kind_t { global, function, loop, block } kind;
        std::map<name_id, symbol_id> symbols;
    };

    struct function {
        fn_id id;
        std::vector<type> param_types;
        type return_type;
    };

    struct enumeration {
        enum_id id;
        // TODO
    };

    struct structure {
        struct_id id;
        // TODO;
    };

    struct symbol {
        symbol_id id;
        src_location src_loc;
        enum kind_t { variable, function, _enum, _struct } kind;
        enum mod_t { mut, imut } modifier;
        enum storage_t { global, local } storage;
        // TODO: probably represent type here also ???
    };

    struct analysis_result {
        std::vector<std::string> names;
        std::vector<symbol> symbols;
        std::vector<enumeration> enumerations;
        std::vector<structure> structures;
        std::vector<function> functions;
        std::vector<scope> scopes;
    };

    struct semantic_analyzer {
        analysis_result semantics;
        std::map<std::string, name_id, std::less<> > interned_names;

        // TODO
        structure create_structure() {
            struct_id id{.value = static_cast<uint32_t>(semantics.structures.size())};
            return {.id = id};
        }

        // TODO
        enumeration create_enumeration() {
            enum_id id{.value = static_cast<uint32_t>(semantics.enumerations.size())};
            enumeration e = { .id = id };
            semantics.enumerations.push_back(e);
            return e;
        }

        function create_function() {
            fn_id id{.value = static_cast<uint32_t>(semantics.functions.size())};
            function fn = {.id= id };
            semantics.functions.push_back(fn);
            return fn;
        }

        scope create_scope(std::optional<scope_id> parent_scope, std::optional<fn_id> enclosing_fn,
                           scope::kind_t kind) {
            scope_id id{.value = static_cast<uint32_t>(semantics.scopes.size())};
            // TODO: we (prolly) don't need to create a copy here ...
            scope curr = {.id = id, .parent_scope = parent_scope, .enclosing_fn = enclosing_fn, .kind = kind};
            semantics.scopes.push_back(curr);
            return curr;
        }

        // TODO: additional fields.
        symbol create_symbol(symbol::kind_t kind) {
            symbol_id sid{.value = static_cast<uint32_t>(semantics.symbols.size())};
            symbol sym = {.id = sid, .kind = kind};
            semantics.symbols.push_back(sym);
            return sym;
        }

        enumeration &get_enumeration(enum_id id) { return semantics.enumerations.at(id.value); }
        function &get_function(fn_id fid) { return semantics.functions.at(fid.value); }
        scope &get_scope(scope_id sid) { return semantics.scopes.at(sid.value); }

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

        void declare(std::string_view name, symbol::kind_t cat, scope_id sid) {
            const auto &[_, nid] = intern_name(name);
            scope &scope = get_scope(sid);
            symbol sym = create_symbol(cat);

            if (scope.symbols.contains(nid))
                diag::error("Symbol already present", sym.src_loc );

            scope.symbols[nid] = sym.id;
        }

        std::optional<symbol_id> lookup_symbol(std::string_view name, scope_id sid) {
            const auto &[inserted, nid] = intern_name(name);
            if (inserted)
                return {};

            std::optional<scope_id> curr_id = sid;
            while (curr_id) {
                scope &curr = get_scope(*curr_id);
                auto it = curr.symbols.find(nid);
                if (it != curr.symbols.end())
                    return it->second;
                curr_id = curr.parent_scope;
            }
            return {};
        }

        bool inside_loop(scope_id curr_id) {
            while (semantics.scopes.at(curr_id.value).kind != scope::loop) {
                auto next_id = semantics.scopes.at(curr_id.value).parent_scope;
                if (!next_id)
                    return false;
                curr_id = next_id.value();
            }
            return true;
        }

        void resolve_expr(const ast::expr &expr, scope_id scope) {
            if (auto nld = std::get_if<ast::num_lit_data>(&expr.data)) {
            } else if (auto bld = std::get_if<ast::bool_lit_data>(&expr.data)) {
            } else if (auto fld = std::get_if<ast::float_lit_data>(&expr.data)) {
            } else if (auto sld = std::get_if<ast::string_lit_data>(&expr.data)) {
            } else if (auto id = std::get_if<ast::identifier_data>(&expr.data)) {
            } else if (auto ud = std::get_if<ast::unary_data>(&expr.data)) {
            } else if (auto bd = std::get_if<ast::binary_data>(&expr.data)) {
            } else if (auto rd = std::get_if<ast::relational_data>(&expr.data)) {
            } else if (auto ad = std::get_if<ast::assign_data>(&expr.data)) {
            } else if (auto cd = std::get_if<ast::call_data>(&expr.data)) {
            } else if (auto fad = std::get_if<ast::field_access_data>(&expr.data)) {
            } else if (auto aid = std::get_if<ast::array_index_data>(&expr.data)) {
            } else if (auto ifd = std::get_if<ast::if_data>(&expr.data)) {
            } else if (auto wd = std::get_if<ast::while_data>(&expr.data)) {
            } else if (auto ld = std::get_if<ast::loop_data>(&expr.data)) {
            } else if (auto md = std::get_if<ast::match_data>(&expr.data)) {
            } else if (auto sd = std::get_if<ast::struct_literal_data>(&expr.data)) {
            } else if (auto blk = std::get_if<ast::block_data>(&expr.data)) {
            } else
                assert(false && "Non-exhaustive data cases!");
        }

        void resolve_stmt(const ast::stmt &stmt, scope_id scope) {
            if (auto ld = std::get_if<ast::let_data>(&stmt.data)) {
            } else if (auto rd = std::get_if<ast::ret_data>(&stmt.data)) {
            } else if (auto cd = std::get_if<ast::cont_data>(&stmt.data)) {
            } else if (auto bd = std::get_if<ast::brk_data>(&stmt.data)) {
            } else if (auto exp = std::get_if<ast::expr_stmt_data>(&stmt.data)) {
            } else
                assert(false && "Non-exhaustive data cases!");
        }

        // TODO: be very cautious of dereferencing null pointers in the following functions..
        //         where should we assert that we don't expect nullptrs and where are they acceptable( prolly ret and if )...
        // TODO: add current function in the error diagnostics
        void collect_declarations(ast::expr &expr, scope_id sid) {
            if (auto ud = std::get_if<ast::unary_data>(&expr.data)) {
                collect_declarations(*ud->lhs, sid);
            } else if (auto bd = std::get_if<ast::binary_data>(&expr.data)) {
                collect_declarations(*bd->lhs, sid);
                collect_declarations(*bd->rhs, sid);
            } else if (auto rd = std::get_if<ast::relational_data>(&expr.data)) {
                collect_declarations(*bd->lhs, sid);
                collect_declarations(*bd->rhs, sid);
            } else if (auto ad = std::get_if<ast::assign_data>(&expr.data)) {
                collect_declarations(*ad->val, sid);
            } else if (auto ifd = std::get_if<ast::if_data>(&expr.data)) {
                collect_declarations(*ifd->then_body, sid);
                if (ifd->else_body)
                    collect_declarations(*ifd->else_body, sid);
            } else if (auto wd = std::get_if<ast::while_data>(&expr.data)) {
                scope while_scope = create_scope(sid, get_scope(sid).enclosing_fn, scope::loop);
                collect_declarations(*wd->body, while_scope.id);
            } else if (auto ld = std::get_if<ast::loop_data>(&expr.data)) {
                scope loop_scope = create_scope(sid, get_scope(sid).enclosing_fn, scope::loop);
                collect_declarations(*ld->body, loop_scope.id);
            } else if (auto blk = std::get_if<ast::block_data>(&expr.data)) {
                scope block_scope = create_scope(sid, get_scope(sid).enclosing_fn, scope::block);
                for (auto &stmt: blk->stmts)
                    collect_declarations(*stmt, block_scope.id);
                if (blk->trailing)
                    collect_declarations(*blk->trailing, block_scope.id);
            }
        }

        void collect_declarations(ast::stmt &stmt, scope_id scope) {
            if (auto ld = std::get_if<ast::let_data>(&stmt.data)) {
                collect_declarations(*ld->decl.initializer, scope);
                declare(ld->decl.name, symbol::variable, scope);
            } else if (auto rd = std::get_if<ast::ret_data>(&stmt.data)) {
                if (rd->val)
                    collect_declarations(*rd->val, scope);
            } else if (auto cd = std::get_if<ast::cont_data>(&stmt.data)) {
                if (!inside_loop(scope))
                    diag::error("Continue used outside of a loop", stmt.src_loc);
            } else if (auto bd = std::get_if<ast::brk_data>(&stmt.data)) {
                if (!inside_loop(scope))
                    diag::error("Break used outside of a loop", stmt.src_loc);
            } else if (auto exp = std::get_if<ast::expr_stmt_data>(&stmt.data)) {
                if (exp->expr)
                    collect_declarations(*exp->expr, scope);
            } else
                assert(false && "Non-exhaustive data cases!");
        }

        void collect_declarations(const ast::module &module) {
            scope global = create_scope(std::nullopt, std::nullopt, scope::global);
            for (const auto &toplevel: module.toplevel_items) {
                if (auto fd = std::get_if<ast::fn_decl>(&toplevel.data)) {
                    function f = create_function();
                    scope fn_scope = create_scope(global.id, f.id, scope::function);
                    if (fd->body)
                        collect_declarations(*fd->body, fn_scope.id);
                } else if (auto ed = std::get_if<ast::enum_decl>(&toplevel.data)) {
                    // TODO: introduce this into the language later
                } else if (auto sd = std::get_if<ast::struct_decl>(&toplevel.data)) {
                    // TODO: introduce this into the language later
                } else if (auto gvd = std::get_if<ast::global_var_decl>(&toplevel.data)) {
                    collect_declarations( *gvd->initializer, global.id );
                    declare(gvd->name, symbol::variable, global.id);
                } else
                    assert(false && "Non-exhaustive data cases!");
            }
        }

        analysis_result run(const ast::module &module) {
            std::cout << "Running semantic analysis\n";
            collect_declarations(module);
            std::cout << "collected declaration\n";
            return semantics;
        }
    };
}
