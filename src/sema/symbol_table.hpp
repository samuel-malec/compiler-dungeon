#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "types.hpp"

namespace dungeon::sema {
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
        std::vector<const type *> param_types;
        const type *return_type;
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

        const type *ty;
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
        type_manager types;
        std::unordered_map<ast::expr *, const type *> expr_ty;
        std::unordered_map<ast::expr *, const function *> expr_fn;

        std::vector<symbol> symbols;

        // TODO:: fill these two
        std::unordered_map<const ast::expr *, symbol_id> id_symbols;
        std::unordered_map<const ast::var_decl *, symbol_id> var_decl_symbols;

        std::vector<std::string> names;
        std::map<std::string, name_id, std::less<> > interned_names;

        std::vector<enumeration> enumerations;
        std::vector<structure> structures;
        std::vector<function> functions;
        std::vector<scope> scopes;
    };
}
