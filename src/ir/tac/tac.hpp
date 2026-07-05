#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../../frontend/types.hpp"

namespace dungeon::tac
{

// todo: copy and goto and call? 

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
using tmp_id = std::string;

using argument = std::variant< tmp_id, constant >;

enum op_kind
{
    unary,
    binary,
    copy,
    jump,
    branch_if,
    param,
    call,
    ret,
    label,
};

struct unary_data
{
    un_op op;
    argument arg1;
};

struct binary_data
{
    bin_op op;
    argument arg1;
    argument arg2;
};

struct copy_data{};

struct jump_data
{

};

struct branch_if_data{};
struct param_data{};
struct call_data{};

struct ret_data
{
    std::optional< argument > arg;
};

struct label_data
{
    std::string name;
};

struct instr
{
    op_kind kind; 
    argument arg1;
    argument arg2;
    tmp_id target;
};

using instr_ptr = std::unique_ptr< instr >;

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