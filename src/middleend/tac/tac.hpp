#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../../frontend/types.hpp"

namespace dungeon::tac
{

using value_id = uint32_t;
using label_id = uint32_t;
using fn_id = uint32_t;

struct value
{
    value_id id;
};

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