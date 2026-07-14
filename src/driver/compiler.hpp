#pragma once

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept> 

#include "config.hpp"

namespace dungeon
{

struct compiler
{
    void run( config& conf );
};

}
