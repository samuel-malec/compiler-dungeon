#pragma once
#include "pass.hpp"

namespace dungeon {
    struct simplify_cfg : pass {
        static void mark_reachable(basic_block *bb, std::unordered_set<basic_block *> &reachable) {
            reachable.insert(bb);
            for (basic_block *ptr: bb->succ)
                if (!reachable.contains(ptr))
                    mark_reachable(ptr, reachable);
        }

        static bool remove_unreachable_blocks(ir::function &fn) {
            std::unordered_set<basic_block *> reachable;
            mark_reachable(fn.entry, reachable);
            bool changed = false;

            for (auto &block: fn.blocks) {
                if (std::erase_if(block->succ, [&](basic_block *s) { return !reachable.contains(s); }))
                    changed = true;

                if (std::erase_if(block->pred, [&](basic_block *p) { return !reachable.contains(p); }))
                    changed = true;

                for (auto inst: block->instructions) {
                    if (inst->op != ir::opcode::phi)
                        continue;

                    auto &incoming = std::get<ir::phi_data>(inst->data).incoming;
                    if (std::erase_if(incoming, [&](const auto &kv) {
                        bool contains = reachable.contains(kv.first);
                        if (!contains)
                            inst->erase_use(kv.second);
                        return !contains;
                    })) {
                        changed = true;
                        inst->operands.clear();
                        for (auto &[blk, val]: incoming)
                            inst->operands.push_back(val);
                    }
                }
            }

            if (std::erase_if(fn.blocks, [&](const std::unique_ptr<basic_block> &b) {
                return !reachable.contains(b.get());
            }))
                changed = true;

            return changed;
        }

        static bool simplify_terminators(ir::function &fn) {
            bool changed = false;
            for (auto &block: fn.blocks) {
                if (!block->has_terminator())
                    continue;
                auto *term = block->instructions.back();
                if (term->op != ir::opcode::cond_br)
                    continue;

                auto cbr = std::get<ir::cond_br_data>(term->data);
                if (cbr.false_branch != cbr.true_branch)
                    continue;

                auto true_succ = block->succ[0];
                term->erase_operands();
                term->op = ir::opcode::br;
                term->data = ir::br_data{.branch_id = cbr.true_branch};
                block->succ.clear();
                block->succ.push_back(true_succ);
                changed = true;
            }

            return changed;
        }

        static bool merge_blocks(ir::function &fn) {
            bool changed = false;
            for (auto &block: fn.blocks) {
                basic_block *pred = block.get();
                if (pred->succ.size() != 1)
                    continue;

                basic_block *succ = pred->succ[0];
                if (succ == pred)
                    continue;
                if (succ->pred.size() != 1 || succ->pred[0] != pred)
                    continue;

                if (pred->has_terminator()) {
                    assert(pred->instructions.back()->op == ir::opcode::br);
                    pred->instructions.pop_back();
                }

                for (auto inst: succ->instructions) {
                    inst->parent = pred;
                    pred->instructions.push_back(inst);
                }

                pred->succ = succ->succ;
                for (auto s: succ->succ) {
                    std::ranges::replace(s->pred, succ, pred);
                    for (auto phi_inst: s->instructions) {
                        if (phi_inst->op != ir::opcode::phi)
                            continue;
                        auto &incoming = std::get<ir::phi_data>(phi_inst->data).incoming;
                        if (auto it = incoming.find(succ); it != incoming.end()) {
                            auto val = it->second;
                            incoming.erase(it);
                            incoming[pred] = val;
                        }
                    }
                }

                succ->instructions.clear();
                succ->succ.clear();
                changed = true;
            }
            return changed;
        }

        static bool simplify_phis(ir::function &fn) {
            bool changed = false;
            for (auto &block: fn.blocks) {
                for (auto &ins: block->instructions) {
                    if (ins->op != ir::opcode::phi)
                        continue;

                    auto pd = std::get<ir::phi_data>(ins->data);
                    if (pd.incoming.size() > 1)
                        continue;

                    assert(pd.incoming.size() == 1);
                    auto val = pd.incoming.begin()->second;
                    ins->erase_operands();
                    replace_all_uses_with(ins->result, val);
                    std::erase_if(block->instructions, [&](ir::instruction *i) { return i == ins; });
                    changed = true;
                }
            }

            return changed;
        }

        static bool remove_trivial_blocks(ir::function &fn) {
            bool changed = false;
            for (auto &block: fn.blocks) {
                if (block->instructions.size() != 1 || block->instructions[0]->op != ir::opcode::br)
                    continue;

                if (block->pred.empty() && fn.entry != block.get())
                    continue;

                assert(block->succ.size() == 1);
                auto succ = block->succ[0];

                for (auto inst: succ->instructions) {
                    if (inst->op != ir::opcode::phi) continue;
                    auto &incoming = std::get<ir::phi_data>(inst->data).incoming;
                    auto it = incoming.find(block.get());
                    if (it == incoming.end()) continue;
                    ir::value *val = it->second;
                    incoming.erase(it);
                    for (auto pred: block->pred) {
                        incoming[pred] = val;
                        val->users.push_back(inst);
                    }
                }

                std::erase(succ->pred, block.get());

                for (auto pred: block->pred) {
                    succ->pred.push_back(pred);
                    pred->replace_successor(block.get(), succ);
                }

                if (fn.entry == block.get())
                    fn.entry = succ;

                changed = true;
            }

            return changed;
        }

        void run(ir::function &fn) override {
            bool changed = true;
            while (changed) {
                changed = false;
                changed |= remove_unreachable_blocks(fn);
                changed |= simplify_terminators(fn);
                changed |= merge_blocks(fn);
                changed |= simplify_phis(fn);
                changed |= remove_trivial_blocks(fn);
            }
        }

        std::string description() override {
            return "Simplify control flow graph structure";
        }
    };
}
