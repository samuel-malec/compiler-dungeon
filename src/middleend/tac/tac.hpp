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
using tmp_id = uint32_t;

using constant = std::variant< uint64_t, bool >;   

struct tmp { tmp_id id; };
struct var 
{
    uint32_t source_name_idx; // idx to symtab, source code name of the variable, eg "x"
    var_id id; // unique id to each occurence of the same variable name in a function, like "x0", "x1"
};

using loc = std::variant< tmp, var >;
using argument = std::variant< loc, constant >;

struct unary_data
{
    dungeon::op_kind op;
    argument arg1;
    loc target;
};

struct binary_data
{
    dungeon::op_kind op;
    argument arg1;
    argument arg2;
    loc target;
};

struct copy_data
{
    argument arg1;
    loc target;
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
    loc target;
};

struct call_data
{
    fn_id callee;
    int args;
    loc target;
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