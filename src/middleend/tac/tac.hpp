#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../../frontend/types.hpp"

namespace dungeon::tac
{

using label_id = uint32_t;
using fn_id = uint32_t;
using var_id = uint32_t;
using constant = std::variant< uint64_t, bool >;   

struct tmp { int id; };
using argument = std::variant< tmp, constant >;

enum instr_kind
{
    unary,
    binary,
    copy,
    jump,
    branch,
    param,
    get_param,
    call,
    ret,
    label,
};

struct unary_data
{
    dungeon::op_kind op;
    argument arg1;
    tmp target;
};

struct binary_data
{
    dungeon::op_kind op;
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
    label_id label;
};

struct branch_data
{
    argument arg1;
    label_id true_lab;
    label_id false_lab;
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
    fn_id callee;
    int args;
    tmp target;
};

struct label_data
{
   label_id id;
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
                    branch_data,
                    param_data,
                    get_param_data,
                    call_data,
                    label_data,
                    ret_data>;
    data_type data;
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