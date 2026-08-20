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

struct symbol_id { uint32_t value; };
struct binding_id { uint32_t value; };
struct fn_id { uint32_t value; };
struct enum_id { uint32_t value; };
struct struct_id { uint32_t value; };
struct scope_id { uint32_t value; };

struct function
{
    fn_id id;
};

struct enumeration
{
    enum_id id;
};

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

struct symbol
{
    symbol_id id;
    enum cat_t { variable, function, _enum, _struct } cat;

};

struct analysis_result
{
    std::map< ast::stmt_ptr, symbol > symbols;
    std::map< symbol_id, function > functions;
    std::map< symbol_id, enumeration > enums;
};

struct semantic_analyzer
{
    analysis_result semantics;

    void run(const ast::module& module) {
        std::cout << "Running semantic analysis\n";
    }
};

}
