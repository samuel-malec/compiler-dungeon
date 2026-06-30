#pragma once

#include <vector>
#include <ostream>

namespace dungeon
{

// how to represent types ? 
enum prim_type
{
    INT,
    CHAR,
    BOOL,
    VOID,
    UNKNOWN,
};

inline std::ostream& operator<<( std::ostream& os, const prim_type pt )
{
    switch ( pt )
    {
        case INT:       return os << "int";
        case BOOL:      return os << "bool";
        case VOID:      return os << "void";
        case UNKNOWN:   return os << "unknown";
    }

    return os << "idk";
}

struct fn_signature
{
    prim_type ret_type;
    std::vector< prim_type > param_types;    
};

// TODO: type resulution

}