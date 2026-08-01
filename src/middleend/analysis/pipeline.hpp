#pragma once

#include <vector>

namespace dungeon::analysis
{

enum pass
{
    TRANSFORM_TO_SSA,
    DEAD_CODE_ELIM,
    LIVENESS_ANALYSIS,
    CONSTANT_PROPAGATION,
    STRENGTH_REDUCTION,
    /**
     * 
     * TODO
     * 
     */
    TRANSFORM_OUT_OF_SSA,
};

static std::vector< pass > pasess = { TRANSFORM_TO_SSA, TRANSFORM_OUT_OF_SSA };

void run_pipeline()
{
    // TODO
    // out_of_ssa()
}
}
