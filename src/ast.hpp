#pragma once

#include <vector>
#include <string>

namespace dungeon
{

struct no_copy
{
    no_copy() = default;
    no_copy( const no_copy& ) = delete;
    no_copy( no_copy&& ) = default;
    no_copy& operator=( no_copy&& ) noexcept = default;
};

struct expr : no_copy
{
    enum cat_t
    {
        num_lit,
        bool_lit,
        identifier,
        unary,
        binary,
        relational,
        assign,
        call,
    } cat;

    std::vector< expr > subs{};
};    


struct stmt : no_copy
{
    enum cat_t
    {
        ret,
        if_stmt,
        block,
        expr_stmt,
    } cat;

    std::vector< stmt > subs{};
};


struct fn_decl
{
    std::string name;
    std::vector< stmt > body;
};

}