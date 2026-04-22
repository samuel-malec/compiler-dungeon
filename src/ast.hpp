#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <vector>

namespace dungeon::ast {

// struct no_copy 
// {
//     no_copy() = default;
//     no_copy(const no_copy &) = delete;
//     no_copy(no_copy &&) = default;
//     no_copy &operator=(no_copy &&) noexcept = default;
// };

enum type_kind
{
    INT,
    BOOL,
    UNSIGNED,
    VOID,
    UNKNOWN,
};

inline std::ostream& operator<<( std::ostream& os, const type_kind tk )
{
    switch ( tk )
    {
        case INT:       return os << "int";
        case BOOL:      return os << "bool";
        case UNSIGNED:  return os << "unsigned";
        case VOID:      return os << "void";
        case UNKNOWN:   return os << "unknown";
    }

    return os << "idk";
}

enum op_kind 
{
    ADD,
    SUB,
    MUL,
    DIV,
    REM,
    SHL,
    SHR,
};

struct expr 
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

    std::variant< std::monostate, int64_t, bool, std::string > data;
    std::vector< expr > subs{};
    type_kind type;
    op_kind op;
    expr &left();
    expr &right();
};

struct var_decl
{
    std::string_view name;
    type_kind type;
    std::optional< expr >  e;
};

struct stmt 
{
    enum cat_t 
    {
        ret,
        if_stmt,
        for_stmt,
        while_stmt,
        do_while_stmt,
        cont,
        brk,
        block,
        var_dclr,
        var_assign,
        expr_stmt,
    } cat;

    std::vector< stmt > subs{};
    expr e;
    var_decl vdecl;

    stmt& operator[]( int n )
    {
        assert( n < subs.size() );
        return subs[ n ];
    }
};

// todo: rewrite this into separate subclasses cuz these don't have much in common ig
struct decl 
{
    enum cat_t
    {
        enum_decl,
        struct_decl,
        fn_decl,
    } cat;

    type_kind type;
    std::string_view name;
    expr e;
    stmt body;
    std::vector< var_decl > vars; 
};

struct program 
{
    std::vector< decl > declarations;
};

} // namespace dungeon
