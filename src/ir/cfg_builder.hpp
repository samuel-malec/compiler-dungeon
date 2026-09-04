#pragma once
#include <functional>

#include "function.hpp"
#include <algorithm>
#include <unordered_map>

namespace dungeon::ir {
    struct cfg_builder {
        void connect(basic_block *from, basic_block *to) {
            if (std::find(from->succ.begin(), from->succ.end(), to) == from->succ.end())
                from->succ.push_back(to);
            if (std::find(to->pred.begin(), to->pred.end(), from) == to->pred.end())
                to->pred.push_back(from);
        }

        void build(module &mod) {
            for (auto &fn: mod.funcs)
                build(fn);
        }

        void build(function &fn) {
            if (fn.instructions.empty())
                return;

            std::vector<size_t> leaders{0};
            std::unordered_map<uint32_t, size_t> label_positions;
            for (size_t i = 0; i < fn.instructions.size(); ++i) {
                const instruction *ins = fn.instructions[i].get();
                if (ins->op == opcode::label)
                    label_positions.emplace(std::get<label_data>(ins->data).id, i);
                if (is_terminator(ins) && i + 1 < fn.instructions.size())
                    leaders.push_back(i + 1);
            }
            for (const auto &[_, position]: label_positions)
                leaders.push_back(position);

            std::sort(leaders.begin(), leaders.end());
            leaders.erase(std::unique(leaders.begin(), leaders.end()), leaders.end());

            std::unordered_map<uint32_t, basic_block *> labels;
            for (size_t i = 0; i < leaders.size(); ++i) {
                const size_t begin = leaders[i];
                const size_t end = i + 1 < leaders.size() ? leaders[i + 1] : fn.instructions.size();
                auto block = std::make_unique<basic_block>();
                block->id = block_id{static_cast<uint32_t>(fn.blocks.size())};
                for (size_t j = begin; j < end; ++j) {
                    const instruction *ins = fn.instructions[j].get();
                    if (ins->op == opcode::label)
                        labels.emplace(std::get<label_data>(ins->data).id, block.get());
                    else
                        block->instructions.push_back(const_cast<instruction *>(ins));
                }
                fn.blocks.push_back(std::move(block));
            }

            for (size_t i = 0; i < fn.blocks.size(); ++i) {
                basic_block *block = fn.blocks[i].get();
                if (block->instructions.empty()) {
                    if (i + 1 < fn.blocks.size())
                        connect(block, fn.blocks[i + 1].get());
                    continue;
                }
                const instruction *last = block->instructions.back();
                if (last->op == opcode::br) {
                    connect(block, labels.at(std::get<br_data>(last->data).branch_id));
                } else if (last->op == opcode::cond_br) {
                    const auto &data = std::get<cond_br_data>(last->data);
                    connect(block, labels.at(data.true_branch));
                    connect(block, labels.at(data.false_branch));
                } else if (last->op != opcode::ret && i + 1 < fn.blocks.size()) {
                    connect(block, fn.blocks[i + 1].get());
                }
            }

            fn.entry = fn.blocks.front().get();
        }
    };
}
