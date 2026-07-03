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

    std::string describe() const
    {
        std::string out = "fn(";
        for ( int i = 0; i < params.size(); ++i )
        {
            std::ostringstream oss;
            oss << params[ i ];
            out += oss.str();
        }
        out += ") -> ";
        std::ostringstream ret;
        ret << ret_type;
        out += ret.str();
        return out;
    }
};

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
        if ( is_primitive() )
        {
            std::ostringstream oss;
            oss << as_primitive();
            return oss.str();
        }
        return as_function().describe();
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

}
