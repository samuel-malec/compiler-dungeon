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
        std::map<basic_block *, ir::value *> incoming;
    };

    struct terminator {
    };

    struct basic_block {
        block_id id;
        uint32_t postorder_id;

        std::vector<phi_node> phis;
        std::vector<ir::instruction *> instructions;

        std::vector<basic_block *> succ;
        std::vector<basic_block *> pred;

        basic_block *idom = nullptr;
        std::vector<basic_block *> df;
        std::vector<basic_block *> dom_children;

        bool has_terminator() const {
            return !instructions.empty() && instructions.back()->is_terminator();
        }

        void replace_successor(basic_block *old_succ, basic_block *new_succ) {
            for (int i = 0; i < succ.size(); ++i)
                if (succ[i] == old_succ)
                    succ[i] = new_succ;

            assert(!instructions.empty());
            auto terminator = instructions.back();
            if (terminator->op == ir::opcode::br) {
                auto brd = std::get<ir::br_data>(terminator->data);
                brd.branch_id = new_succ->id.id;
                return;
            }

            assert(terminator->op == ir::opcode::cond_br);
            auto cbrd = std::get<ir::cond_br_data>(terminator->data);
            if (cbrd.true_branch == old_succ->id.id) {
                cbrd.true_branch = new_succ->id.id;
            } else {
                cbrd.false_branch = new_succ->id.id;
            }
        }
    };

    struct edge {
        basic_block *from;
        basic_block *to;

        bool operator==(const edge &o) const {
            return from == o.from && to == o.to;
        }

        bool operator<(const edge &o) const {
            if (from == o.from)
                return to < o.to;
            return from < o.from;
        }
    };
}
