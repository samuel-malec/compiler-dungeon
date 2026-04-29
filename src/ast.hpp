#pragma once

#include <cassert>
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>
#include <optional>
#include <vector>

namespace dungeon::ast {

// TODO: it would be nice to have non-copyable expressions and statements, but it would break a lot of things rn ( we need to get things working first )
// struct no_copy 
// {
//     no_copy() = default;
//     no_copy(const no_copy &) = delete;
//     no_copy(no_copy &&) = default;
//     no_copy &operator=(no_copy &&) noexcept = default;
// };

// FIXME: type info prolly shouldn't be in the ast but whateva, I definitely won't forget about this and refactor it. 
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
    ADD, SUB, MUL, DIV,
    REM, SHL, SHR,

    EQ, NEQ, LE, LEQ, GE, GEQ,

    NOT, AND, OR,
};

inline std::ostream& operator<<( std::ostream& os, const op_kind op )
{
    switch ( op )
    {
        case ADD:   return os << "+";
        case SUB:   return os << "-";
        case MUL:   return os << "*";
        case DIV:   return os << "/";
        case REM:   return os << "%";
        case SHL:   return os << "<<";
        case SHR:   return os << ">>";
        case EQ:    return os << "==";
        case NEQ:   return os << "!=";
        case LE:    return os << "<";
        case LEQ:   return os << "<=";
        case GE:    return os << ">";
        case GEQ:   return os << ">=";
        case NOT:   return os << "!";
        case AND:   return os << "&&";
        case OR:    return os << "||";
    }

    return os << "idk";
}

struct expr;
struct stmt;
struct fn_decl;
struct var_decl;

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

    enum val_kind_t
    {
        lvalue,
        rvalue,
    } val_kind;

    std::variant< std::monostate, uint64_t, bool, std::string > val;
    std::string_view id;
    std::vector< expr > subs{};
    type_kind type;
    op_kind op;
    
    expr &left() 
    { 
        assert( !subs.empty() );
        return subs[ 0 ];
    }

    expr &right() 
    { 
        assert( 1 < subs.size() );
        return subs[ 1 ];
    }

    expr& operator[]( int n )
    {
        assert( n < subs.size() );
        return subs[ n ];
    }
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
        expr_stmt,
    } cat;

    std::vector< stmt > subs{};
    std::optional< expr > e;
    var_decl vdecl;

    stmt& operator[]( int n )
    {
        assert( n < subs.size() );
        return subs[ n ];
    }
};

struct enum_decl{};

struct struct_decl{};

// TODO: struct function signature should be here instead 
struct fn_decl
{
    type_kind sig_type;
    std::string_view name;
    std::vector< stmt > body;
    std::vector< var_decl > params;
};

using decl = std::variant< fn_decl, enum_decl, struct_decl >;

struct program 
{
    std::vector< decl > decls;
};

} // namespace dungeon
