#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "instruction.hpp"

namespace dungeon {
    struct block_id {
        uint32_t id;
    };

    struct phi_node {
        uint32_t base_id;
        ir::value *res;
        std::unordered_map<block_id, ir::value *> incoming;
    };

    struct terminator {
    };

    struct basic_block {
        block_id id;
        uint32_t postorder_id;

        std::vector<phi_node> phis;
        std::vector<ir::instruction *> instructions;
        std::unique_ptr<terminator> terminator;

        std::vector<basic_block *> succ;
        std::vector<basic_block *> pred;

        basic_block *idom = nullptr;
        std::vector<basic_block *> df;
        std::vector<basic_block *> dom_children;
    };
}
