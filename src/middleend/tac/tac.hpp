#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../../frontend/types.hpp"

// TODO: this was designed before ssa and everything was beautiful,
// now we are forced to do this variant bs, I will probably rewrite instruction into a single struct
// without variant data once I have time.
namespace dungeon::tac
{

using value_id = uint32_t;
using label_id = uint32_t;
using fn_id = uint32_t;

struct value
{
    value_id id;
    uint32_t version = 0;
};

inline bool operator<( const value& lhs, const value& rhs )
{
    return lhs.id != rhs.id ? lhs.id < rhs.id : lhs.version < rhs.version;
}

using constant = std::variant< uint64_t, bool >;   
using operand = std::variant< constant, value >;

struct unary_data
{
    dungeon::op_kind op;
    operand arg1;
    value target;
};

struct binary_data
{
    dungeon::op_kind op;
    operand arg1;
    operand arg2;
    value target;
};

struct copy_data
{
    operand arg1;
    value target;
};

struct jump_data
{
    label_id label;
};

struct branch_data
{
    operand arg1;
    label_id true_lab;
    label_id false_lab;
};

struct param_data
{
    operand arg;
};

struct get_param_data
{
    int idx;
    value target;
};

struct call_data
{
    fn_id callee;
    int args;
    value target;
};

struct label_data
{
   label_id id;
};

struct ret_data
{
    std::optional< operand > arg;
};

struct instr
{
    using data_type = std::variant< 
                        unary_data,
                        binary_data,
                        copy_data,
                        jump_data,
                        branch_data,
                        param_data,
                        get_param_data,
                        call_data,
                        label_data,
                        ret_data >;
    data_type data;

    void for_each_use( auto&& f )   // f(value&) called on every read operand
    {
        auto visit_operand = [ & ]( operand& o )
        {
            if ( auto* v = std::get_if< value >( &o ) )
                f( *v );
        };

        std::visit( [ & ]( auto&& d )
        {
            using T = std::decay_t< decltype( d ) >;
            if constexpr ( std::is_same_v< T, unary_data > )
                visit_operand( d.arg1 );
            else if constexpr ( std::is_same_v< T, binary_data > )
            {
                visit_operand( d.arg1 );
                visit_operand( d.arg2 );
            }
            else if constexpr ( std::is_same_v< T, copy_data > )
                visit_operand( d.arg1 );
            else if constexpr ( std::is_same_v< T, branch_data > )
                visit_operand( d.arg1 );
            else if constexpr ( std::is_same_v< T, param_data > )
                visit_operand( d.arg );
            else if constexpr ( std::is_same_v< T, ret_data > )
            {
                if ( d.arg )
                    visit_operand( *d.arg );
            }
        }, data );
    }

    void set_target( value new_target )
    {
        std::visit( [ & ]( auto&& d )
        {
            using T = std::decay_t< decltype( d ) >;
            if constexpr ( requires { d.target; } )
                d.target = new_target;
        }, data );
    }

    std::optional< value > get_target()
    {
        return std::visit( [ ]( auto&& d ) -> std::optional< value >
        {
            using T = std::decay_t< decltype( d ) >;
            if constexpr ( requires { d.target; } )
                return d.target;
            else
                return std::nullopt;
        }, data );
    }
};

struct function
{
    fn_id name;
    function_type sig;
    std::vector< instr > body;
};

struct program
{
    std::vector< function > functions;
};

};