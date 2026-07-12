#pragma once

#include <ostream>
#include <string>
#include <sstream>
#include <vector>
#include <variant>

namespace dungeon
{

enum prim_type
{
    INT,
    BOOL,
    VOID,
    UNKNOWN,
};

inline std::ostream& operator<<( std::ostream& os, const prim_type& typ )
{
    switch ( typ )
    {
        case INT:
            return os << "int";
        case BOOL:
            return os << "bool";
        case VOID:
            return os << "void";
        default:
            break;
    }
    return os << "UNKNOWN";
}

struct function_type
{
    prim_type ret_type;
    std::vector< prim_type > params;
};

inline std::ostream& operator<< ( std::ostream& os, const function_type& sig )
{
    os << "(";
    for ( int i = 0; i < sig.params.size(); ++i )
        os << sig.params[ i ] << " x ";
    os << ") -> ";
    os << sig.ret_type;
    return os << '\n';
}

inline bool operator==( const function_type& lhs, const function_type& rhs )
{
    return lhs.ret_type == rhs.ret_type && lhs.params == rhs.params;
}

struct type
{
    std::variant< prim_type, function_type > data = prim_type::UNKNOWN;

    bool is_primitive() const { return std::holds_alternative< prim_type >( data ); }

    bool is_function() const { return std::holds_alternative< function_type >( data ); }

    prim_type as_primitive() const { return std::get< prim_type >( data ); }

    function_type as_function() const { return std::get< function_type >( data ); }

    std::string describe() const
    {

        std::ostringstream oss;
        if ( is_primitive() )
            oss << as_primitive();
        else
            oss << as_function();

        return oss.str();
    }
};

inline bool operator==( const type& lhs, const type& rhs )
{
    if ( lhs.is_primitive() && rhs.is_primitive() )
        return lhs.as_primitive() == rhs.as_primitive();

    if ( lhs.is_function() && rhs.is_function() )
    {
        function_type fn_lhs = lhs.as_function();
        function_type fn_rhs = rhs.as_function();
        return fn_lhs.ret_type == fn_rhs.ret_type && fn_lhs.params == fn_rhs.params;
    }

    return false;
}

inline std::ostream& operator<<( std::ostream& os, const type& typ )
{
    return os << typ.describe();
}

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

inline bool is_numerical_op( op_kind op )
{
    return op == ADD || op == SUB || op == MUL ||
           op == DIV || op == MOD || op == SHL || op == SHR;
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
