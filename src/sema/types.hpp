#pragma once

#include <ostream>
#include <string>
#include <sstream>
#include <vector>
#include <variant>

#include "../diag/diag.hpp"

namespace dungeon
{

// TODO: implement types as a singleton, so that every i32 type is only one obect
// TODO: type resolution and etc;
struct type{};

struct int_type : type
{
    bool sign = false;
    size_t width = 8;
};

struct bool_type : type {};

struct type_manager
{

};

//
// enum prim_type
// {
//     INT,
//     BOOL,
//     VOID,
//     UNKNOWN,
// };

enum op_kind
{
    ADD, SUB, MUL, DIV, MOD, SHL, SHR,

    ADD_EQ, SUB_EQ, MUL_EQ, DIV_EQ, MOD_EQ, SHL_EQ, SHR_EQ,

    EQ, NEQ, LT, LEQ, GT, GEQ,

    NOT, AND, OR,
};

inline bool is_rel_op( op_kind op )
{
    return op == EQ || op == NEQ || op == LT || op == LEQ || op == GT || op == GEQ;
}

inline bool is_unary_op( op_kind op )
{
    return false;
}

inline bool is_binary_op( op_kind op )
{
    return false;
}

inline bool is_numerical_op( op_kind op )
{
    return op == ADD || op == SUB || op == MUL ||
           op == DIV || op == MOD || op == SHL || op == SHR;
           op == ADD_EQ || op == SUB_EQ || op == MUL_EQ || op == DIV_EQ || op == MOD_EQ || op == SHL_EQ || op == SHR_EQ;
}

inline bool is_bool_op( op_kind op )
{
    return op == NOT || op == AND || op == OR;
}

inline op_kind op_from_compound_asn( op_kind op )
{
    if ( op == ADD_EQ ) return ADD;
    if ( op == SUB_EQ ) return SUB;
    if ( op == MUL_EQ ) return MUL;
    if ( op == DIV_EQ ) return DIV;
    if ( op == MOD_EQ ) return MOD;
    if ( op == SHL_EQ ) return SHL;
    if ( op == SHR_EQ ) return SHR;

    throw std::runtime_error( "should not reach here, expected a compound assignment!" );
}

inline op_kind op_kind_from_str( std::string_view data )
{
    if ( data == "+"   )   return op_kind::ADD;
    if ( data == "-"   )   return op_kind::SUB;
    if ( data == "*"   )   return op_kind::MUL;
    if ( data == "/"   )   return op_kind::DIV;
    if ( data == "%"   )   return op_kind::MOD;
    if ( data == "<<"  )   return op_kind::SHL;
    if ( data == ">>"  )   return op_kind::SHR;
    if ( data == "=="  )   return op_kind::EQ;
    if ( data == "!="  )   return op_kind::NEQ;
    if ( data == "<"   )   return op_kind::LT;
    if ( data == "<="  )   return op_kind::LEQ;
    if ( data == ">"   )   return op_kind::GT;
    if ( data == ">="  )   return op_kind::GEQ;
    if ( data == "!"   )   return op_kind::NOT;
    if ( data == "&&"  )   return op_kind::AND;
    if ( data == "||"  )   return op_kind::OR;
    if ( data == "="   )   return op_kind::EQ;
    if ( data == "+="  )   return op_kind::ADD_EQ;
    if ( data == "-="  )   return op_kind::SUB_EQ;
    if ( data == "*="  )   return op_kind::MUL_EQ;
    if ( data == "%="  )   return op_kind::MOD_EQ;
    if ( data == "/="  )   return op_kind::DIV_EQ;
    if ( data == "<<=" )   return op_kind::SHL_EQ;
    if ( data == ">>=" )   return op_kind::SHR_EQ;
    diag::error( "Unknown operator:", data );
    return op_kind::ADD;
}

 
inline std::ostream& operator<<( std::ostream& os, const op_kind op )
{
    switch ( op )
    {
        case ADD:    return os << "+";
        case SUB:    return os << "-";
        case MUL:    return os << "*";
        case DIV:    return os << "/";
        case MOD:    return os << "%";
        case SHL:    return os << "<<";
        case SHR:    return os << ">>";
        case EQ:     return os << "==";
        case NEQ:    return os << "!=";
        case LT:     return os << "<";
        case LEQ:    return os << "<=";
        case GT:     return os << ">";
        case GEQ:    return os << ">=";
        case NOT:    return os << "!";
        case AND:    return os << "&&";
        case OR:     return os << "||";
        case ADD_EQ: return os << "+=";
        case SUB_EQ: return os << "-=";
        case MUL_EQ: return os << "*="; 
        case DIV_EQ: return os << "/=";
        case MOD_EQ: return os << "%=";
        case SHL_EQ: return os << "<<=";
        case SHR_EQ: return os << ">>=";
    }

    return os << "idk";
}
}
