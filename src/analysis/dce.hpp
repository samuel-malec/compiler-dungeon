#pragma once
#include <queue>
#include <set>

#include "pass.hpp"

namespace dungeon {
    struct dce : pass {
        std::set<ir::instruction *> get_useful(ir::function &fn) {
            std::set<ir::instruction *> useful;
            std::queue<ir::instruction *> worklist;

            for (auto &block: fn.blocks) {
                for (auto &inst: block->instructions) {
                    if (inst->has_side_effects()) {
                        worklist.push(inst);
                        useful.insert(inst);
                    }
                }
            }

            while (!worklist.empty()) {
                ir::instruction *inst = worklist.front();
                worklist.pop();
                for (auto &operand: inst->operands) {
                    
                    ir::instruction *def_instr = operand->defining_instruction;
                    if (!useful.contains(def_instr)) {
                        worklist.push(def_instr);
                        useful.insert(def_instr);
                    }
                }
            }
            return useful;
        }

        void run(ir::function &fn) override {
            auto useful = get_useful(fn);
            for (auto &block: fn.blocks) {
                auto &instructions = block->instructions;
                std::erase_if(instructions, [&](ir::instruction *inst) { return !useful.contains(inst); });
            }
        }
    };
}
