#pragma once
#include <cstdint>
#include <map>
#include <vector>

#include "instruction.hpp"

namespace dungeon {
    struct block_id {
        uint32_t id;

        bool operator==(const block_id &other) const {
            return id == other.id;
        }

        bool operator<(const block_id &other) const {
            return id < other.id;
        }
    };

    struct phi_node {
        uint32_t base_id;
        ir::value *res;
        std::map<block_id, ir::value *> incoming;
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

        // TODO: We should remove the redundant code
        void remove_succ(const basic_block *to_remove) {
            for (int i = 0; i < succ.size(); i++) {
                if (succ[i] == to_remove) {
                    std::swap(succ[i], succ.back());
                    succ.pop_back();
                }
            }
        }

        void remove_pred(const basic_block *to_remove) {
            for (int i = 0; i < pred.size(); i++) {
                if (pred[i] == to_remove) {
                    std::swap(pred[i], pred.back());
                    pred.pop_back();
                }
            }
        }
    };
}
