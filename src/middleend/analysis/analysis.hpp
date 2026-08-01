#pragma once

#include "../cfg.hpp"

namespace dungeon::analysis
{

struct analysis
{
    virtual void before( cfg& graph ) = 0;

    virtual void run( cfg& graph ) = 0;

    virtual void after( cfg& graph ) = 0; 

    void ~analysis() = default;
};

}
