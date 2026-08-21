#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "types.hpp"
#include "../frontend/ast.hpp"

namespace dungeon
{

struct name_id { uint32_t value; };
struct symbol_id { uint32_t value; };
struct binding_id { uint32_t value; };
struct fn_id { uint32_t value; };
struct enum_id { uint32_t value; };
struct struct_id { uint32_t value; };
struct scope_id { uint32_t value; };

struct scope
{
    scope_id id;
    std::optional< scope_id > enclosing_scope;
    enum cat_t { global, function, loop, block } cat;
};

struct binding
{
    binding_id id;
    scope_id enclosing_scope;
    enum mod_t { mut, imut } modifier;
};

struct function
{
    fn_id id;
    std::vector< binding_id > param_bindings;
};

struct enumeration
{
    enum_id id;
    // TODO
};


struct symbol
{
    name_id id;
    enum cat_t { variable, function, _enum, _struct } cat;
    // TODO: probably represent type here also ???
};

struct analysis_result
{
    std::vector< std::string > names;
    std::vector< symbol > symbols;
    std::vector< enumeration > enumerations;
    std::vector< function > functions;
    std::vector< scope_id > scopes;
    std::vector< binding_id > bindings;


};

struct semantic_analyzer
{
    analysis_result semantics;

    name_id intern_name( std::string_view name ) {

    }

    void declare_expr( const ast::expr& exp ) {}

    void declare_stmt( const ast::stmt& stmt ) {
        if ( auto ld = std::get_if< ast::let_data >( &stmt.data ) ) {}
    }

    void declare_toplevel( const ast::toplevel& toplevel ) {
        if ( auto fd = std::get_if< ast::fn_decl >( &toplevel.data ) ) {}
        else if ( auto ed = std::get_if< ast::enum_decl >( &toplevel.data ) ) {}
        else if ( auto ed = std::get_if< ast::struct_decl >( &toplevel.data ) ) {}
        else if ( auto ed = std::get_if< ast::global_var_decl >( &toplevel.data ) ) {}
    }
    void run(const ast::module& module) {
        std::cout << "Running semantic analysis\n";
        for ( auto& top : module.toplevel_items ) {
            declare_toplevel(top);
        }
    }
};

}
