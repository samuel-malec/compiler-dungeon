#pragma once

#include <cassert>
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>
#include <optional>
#include <vector>

#include "types.hpp"
#include "token.hpp"

namespace dungeon::ast {

// TODO: it would be nice to have non-copyable expressions and statements, but it would break a lot of things rn ( we need to get things working first )
// struct no_copy 
// {
//     no_copy() = default;
//     no_copy(const no_copy &) = delete;
//     no_copy(no_copy &&) = default;
//     no_copy &operator=(no_copy &&) noexcept = default;
// };

enum op_kind 
{
    ADD, SUB, MUL, DIV,
    REM, SHL, SHR,

    EQ, NEQ, LE, LEQ, GE, GEQ,

    NOT, AND, OR,
};

inline bool is_rel_op( op_kind op )
{
    return op == EQ || op == NEQ || op == LE || op == LEQ || op == GE || op == GEQ;
}

inline bool is_numerical_op( op_kind op )
{
    return op == ADD || op == SUB || op == MUL ||
           op == DIV || op == REM || op == SHL || op == SHR;
}

inline bool is_bool_op( op_kind op )
{
    return op == NOT || op == AND || op == OR;
} 

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

/**
 * The current representation of expressions mixes many different things together such as source location, types, value category, etc.
 * We should think about how to decouple this information into separate representations, such as AST and HIR.  
 */
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

    location src_loc;
    std::variant< std::monostate, uint64_t, bool, std::string > val;
    std::string_view id;
    std::vector< expr > subs{};
    type typ;
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
    type typ;
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

    location src_loc;
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

struct fn_decl
{
    location src_loc;
    function_type fn_typ;
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
