#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../../frontend/types.hpp"

namespace dungeon::tac
{

enum class un_op
{
    plus, minus, lnot,
};

enum class bin_op
{
    add, sub, mul, div, mod, shl, shr,
    eq, neq, lt, leq, gt, geq, land, lor,
};

using constant = std::variant< uint64_t, bool >;   
struct tmp
{
    int id;
};

using argument = std::variant< tmp, constant >;

enum op_kind
{
    unary,
    binary,
    copy,
    jump,
    branch_if,
    param,
    get_param,
    call,
    ret,
    label,
};

struct unary_data
{
    un_op op;
    argument arg1;
    tmp target;
};

struct binary_data
{
    bin_op op;
    argument arg1;
    argument arg2;
    tmp target;
};

struct copy_data
{
    argument arg1;
    tmp target;
};

struct jump_data
{
    std::string label;
};

struct branch_if_data
{
    argument arg1;
    std::string label;
};

struct param_data
{
    argument arg;
};

struct get_param_data
{
    int idx;
    tmp target;
};

struct call_data
{
    std::string callee;
    int args;
    tmp target;
};

struct label_data
{
   std::string name;
};

struct ret_data
{
    std::optional< argument > arg;
};

struct instr
{
    using data_type = std::variant< 
                    unary_data,
                    binary_data,
                    copy_data,
                    jump_data,
                    branch_if_data,
                    param_data,
                    get_param_data,
                    call_data,
                    label_data,
                    ret_data>;
    data_type data;
};

struct function
{
    std::string name;
    function_type sig;
    std::vector< instr > body;
};

struct program
{
    std::vector< function > functions;
};

};