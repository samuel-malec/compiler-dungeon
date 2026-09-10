#pragma once
#include <algorithm>
#include <ranges>
#include <set>

#include "pass.hpp"
#include "../diag/diag.hpp"

namespace dungeon {
    struct ssa_builder {
        using value_id = uint32_t;
        using order = std::vector<basic_block *>;
        using stack = std::map<value_id, std::vector<ir::value *> >;
        using var_map = std::map<value_id, ir::value *>;

        uint32_t next_val_id = 1;

        static void dfs(basic_block *bb, order &res, std::set<block_id> &visited) {
            visited.insert(bb->id);
            for (basic_block *ptr: bb->succ)
                if (!visited.contains(ptr->id))
                    dfs(ptr, res, visited);

            res.push_back(bb);
        }

        static void remove_unreachable_blocks(ir::function &fn, const std::set<block_id> &reachable) {
            std::vector<std::unique_ptr<basic_block> > new_blocks;
            for (auto &block: fn.blocks) {
                if (!reachable.contains(block->id))
                    continue;
                new_blocks.push_back(std::move(block));
            }

            fn.blocks = std::move(new_blocks);
        }

        static order reverse_postorder(ir::function &fn) {
            order res{};
            std::set<block_id> visited{};
            dfs(fn.entry, res, visited);

            remove_unreachable_blocks(fn, visited);

            std::ranges::reverse(res);

            for (int i = 0; i < res.size(); ++i)
                res[i]->postorder_id = i;

            return res;
        }

        static basic_block *intersect(basic_block *bb1, basic_block *bb2) {
            while (bb1->postorder_id != bb2->postorder_id) {
                while (bb1->postorder_id > bb2->postorder_id)
                    bb1 = bb1->idom;
                while (bb2->postorder_id > bb1->postorder_id)
                    bb2 = bb2->idom;
            }

            return bb1;
        }

        static void compute_idom_tree(ir::function &fn) {
            auto rpo = reverse_postorder(fn);

            for (auto &bb: fn.blocks)
                bb->idom = nullptr;
            fn.entry->idom = fn.entry;
            bool changed = true;

            while (changed) {
                changed = false;
                for (auto i: rpo) {
                    if (i->id == fn.entry->id)
                        continue;

                    basic_block *b = i;
                    basic_block *new_idom = nullptr;

                    for (basic_block *pred: b->pred) {
                        if (pred->idom != nullptr) {
                            new_idom = pred;
                            break;
                        }
                    }

                    for (basic_block *p: b->pred) {
                        if (p->id == new_idom->id)
                            continue;

                        if (p->idom != nullptr)
                            new_idom = intersect(p, new_idom);
                    }

                    if (b->idom != new_idom) {
                        b->idom = new_idom;
                        changed = true;
                    }
                }
            }

            for (auto &bb: fn.blocks)
                if (bb.get() != fn.entry)
                    bb->idom->dom_children.push_back(bb.get());
        }

        static void compute_dom_frontiers(ir::function &fn) {
            for (auto &bb: fn.blocks) {
                if (bb->pred.size() < 2)
                    continue;

                for (auto &p: bb->pred) {
                    basic_block *runner = p;
                    while (runner != bb->idom) {
                        runner->df.push_back(bb.get());
                        runner = runner->idom;
                    }
                }
            }
        }

        // an alloca is "promotable" if it's a local slot produced by lowering
        // (let-bindings and compiler-generated temporaries); nothing in the
        // frontend can currently take its address, so every alloca qualifies
        static var_map collect_promotable_vars(const ir::function &fn) {
            var_map vars;
            for (auto &bb: fn.blocks)
                for (ir::instruction *ins: bb->instructions)
                    if (ins->op == ir::opcode::alloca)
                        vars[ins->result->id] = ins->result;
            return vars;
        }

        ir::value *create_value(ir::function &fn, const type *ty) {
            auto v = std::make_unique<ir::value>(next_val_id++, ty, std::vector<ir::instruction *>{});
            fn.values.push_back(std::move(v));
            return fn.values.back().get();
        }

        void insert_phi(ir::function &fn, const var_map &vars) {
            std::map<value_id, std::set<basic_block *> > def_blocks;
            for (auto &bb: fn.blocks)
                for (ir::instruction *ins: bb->instructions)
                    if (ins->op == ir::opcode::store && vars.contains(ins->operands[0]->id))
                        def_blocks[ins->operands[0]->id].insert(bb.get());

            for (auto &[vid, defs]: def_blocks) {
                std::set<basic_block *> has_phi;
                std::set on_worklist(defs.begin(), defs.end());
                std::vector worklist(defs.begin(), defs.end());

                while (!worklist.empty()) {
                    basic_block *n = worklist.back();
                    worklist.pop_back();

                    for (basic_block *y: n->df) {
                        if (has_phi.contains(y))
                            continue;

                        y->phis.push_back(phi_node{.base_id = vid, .res = create_value(fn, vars.at(vid)->ty)});
                        has_phi.insert(y);

                        if (!on_worklist.contains(y)) {
                            on_worklist.insert(y);
                            worklist.push_back(y);
                        }
                    }
                }
            }
        }

        // dominator-tree preorder walk: renames loads to the reaching store
        // (or phi), drops promoted alloca/store/load instructions, and wires
        // up phi incoming edges for each successor
        void rename(basic_block *block, stack &s, const var_map &vars) {
            std::map<value_id, int> pushed;

            for (phi_node &phi: block->phis) {
                s[phi.base_id].push_back(phi.res);
                ++pushed[phi.base_id];
            }

            std::vector<ir::instruction *> kept;
            kept.reserve(block->instructions.size());
            for (ir::instruction *ins: block->instructions) {
                if (ins->op == ir::opcode::alloca && vars.contains(ins->result->id))
                    continue;

                if (ins->op == ir::opcode::store && vars.contains(ins->operands[0]->id)) {
                    const value_id vid = ins->operands[0]->id;
                    ir::value *stored = ins->operands[1];
                    erase_use(ins->operands[0], ins);
                    erase_use(stored, ins);
                    s[vid].push_back(stored);
                    ++pushed[vid];
                    continue;
                }

                if (ins->op == ir::opcode::load && vars.contains(ins->operands[0]->id)) {
                    const value_id vid = ins->operands[0]->id;
                    erase_use(ins->operands[0], ins);
                    if (auto &st = s[vid]; !st.empty())
                        replace_all_uses_with(ins->result, st.back());
                    continue;
                }

                kept.push_back(ins);
            }
            block->instructions = std::move(kept);

            for (basic_block *succ: block->succ)
                for (phi_node &phi: succ->phis)
                    if (auto &st = s[phi.base_id]; !st.empty())
                        phi.incoming[block->id] = st.back();

            for (basic_block *child: block->dom_children)
                rename(child, s, vars);

            for (auto &[vid, n]: pushed)
                for (int k = 0; k < n; ++k)
                    s[vid].pop_back();
        }

        void add_phi_instrs(ir::function &fn) {
            for (auto &block: fn.blocks) {
                std::vector<ir::instruction *> new_instrs;

                for (auto &phi: block->phis) {
                    auto i = std::make_unique<ir::instruction>();
                    i->op = ir::opcode::phi;
                    i->result = phi.res;
                    phi.res->defining_instruction = i.get();
                    for (auto &val: phi.incoming | std::views::values) {
                        i->operands.push_back(val);
                        val->users.push_back(i.get());
                    }

                    i->data = ir::phi_data{.incoming = phi.incoming};
                    fn.instructions.push_back(std::move(i));
                    new_instrs.push_back(fn.instructions.back().get());
                }

                block->phis.clear();
                for (auto &old_instr: block->instructions)
                    new_instrs.push_back(old_instr);
                block->instructions = std::move(new_instrs);
            }
        }

        void build(ir::function &fn) {
            for (auto &v: fn.values)
                next_val_id = std::max(next_val_id, v->id + 1);

            compute_idom_tree(fn);
            compute_dom_frontiers(fn);

            const var_map vars = collect_promotable_vars(fn);
            insert_phi(fn, vars);

            stack s{};
            rename(fn.entry, s, vars);
            add_phi_instrs(fn);
        }
    };

    struct mem2reg : pass {
        static void verify_ssa(const ir::function &fn) {
            for (auto &bb: fn.blocks) {
                for (auto &ins: bb->instructions) {
                    if (ins->op != ir::opcode::phi)
                        continue;

                    const auto &data = std::get<ir::phi_data>(ins->data);
                    for (basic_block *pred: bb->pred) {
                        if (!data.incoming.contains(pred->id))
                            diag::error("phi for v", ins->result->id, "in bb", bb->id.id,
                                        "is missing an incoming value from predecessor bb", pred->id.id);
                    }
                }
            }
        }

        void run(ir::function &fn) override {
            ssa_builder sb{};
            sb.build(fn);
            verify_ssa(fn);
        }
    };
}
