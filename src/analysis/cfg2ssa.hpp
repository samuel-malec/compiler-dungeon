#pragma once
#include <algorithm>
#include <set>

#include "pass.hpp"

namespace dungeon {
    struct ssa_builder {
        using value_id = uint32_t;
        using order = std::vector<basic_block *>;

        using version_map = std::map<value_id, uint32_t>;
        using stack = std::map<value_id, std::vector<ir::value> >;

        static void dfs(basic_block *bb, order &res, std::set<block_id> &visited) {
            visited.insert(bb->id);
            for (basic_block *ptr: bb->succ)
                if (!visited.contains(ptr->id))
                    dfs(ptr, res, visited);

            res.push_back(bb);
        }

        static order reverse_postorder(const ir::function &fn) {
            order res{};
            std::set<block_id> visited{};
            dfs(fn.entry, res, visited);
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

        void compute_dom_tree(ir::function& fn) {
            const order rpo = reverse_postorder(fn);

            // computing idoms
            for (auto &bb: fn.blocks)
                bb->idom = nullptr;
            fn.entry->idom = fn.entry;
            bool changed = true;

            while (changed) {
                changed = false;
                for (int i = 0; i < rpo.size(); ++i) {
                    if (rpo[i]->id == fn.entry->id)
                        continue;

                    basic_block *b = rpo[i];
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

        void compute_dom_frontiers(ir::function& fn) {
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

            // for (auto &bb: fn.blocks) {
            //     std::cout << "df( bb " << bb->id << " ) = { ";
            //     for (auto &t: bb->df)
            //         std::cout << t->id << " ";
            //     std::cout << "}\n";
            // }
        }

        void insert_phi(ir::function &fn) {
            std::set<ir::value> vars;
            std::map<uint32_t, std::set<basic_block *> > def_blocks;
            // for (auto &bb: fn.blocks) {
            //     for (auto &ins: bb->instructions) {
            //         ir::value v = *ins->result;
            //         vars.insert(v);
            //         def_blocks[v.id].insert(bb.get());
            //     }
            // }

            for (auto &[vid, defs]: def_blocks) {
                std::set<basic_block *> has_phi;
                std::set on_worklist(defs.begin(), defs.end());
                std::vector worklist(defs.begin(), defs.end());

                while (!worklist.empty()) {
                    const basic_block *n = worklist.back();
                    worklist.pop_back();

                    for (basic_block *y: n->df) {
                        if (has_phi.contains(y))
                            continue;

                        y->phis.push_back(phi_node{.base_id = vid});
                        has_phi.insert(y);

                        if (!on_worklist.contains(y)) {
                            on_worklist.insert(y);
                            worklist.push_back(y);
                        }
                    }
                }
            }
        }

        void rename(basic_block *block, version_map &vm, stack &s) {
            // std::map<value_id, int> pushed;
            // auto fresh = [ & ](value_id id) -> ir::value {
            //     ir::value nv{.id = id, .version = vm[id]++};
            //     s[id].push_back(nv);
            //     pushed[id]++;
            //     return nv;
            // };
            //
            // for (auto &phi: block->phis)
            //     phi.res = fresh(phi.base_id);
            //
            // for (auto &ins: block->instructions) {
            //     ins.for_each_use([ & ](tac::value &v) {
            //         auto &tmp = s[v.id];
            //         if (!tmp.empty())
            //             v = tmp.back();
            //         // empty : use before any reaching def
            //     });
            //
            //     auto d = ins->result;
            //     ins.set_target(fresh(d->id));
            // }
            //
            // for (basic_block *succ: block->succ) {
            //     for (auto &phi: succ->phis) {
            //         if (auto &tmp = s[phi.base_id]; !tmp.empty())
            //             phi.incoming[block->id] = &tmp.back();
            //     }
            // }
            //
            // for (basic_block *child: block->dom_children)
            //     rename(child, vm, s);
            //
            // for (auto &[id, n]: pushed)
            //     for (int k = 0; k < n; ++k)
            //         s[id].pop_back();
        }

        void transform_ssa(ir::function &fn) {
            compute_dom_tree(fn);
            compute_dom_frontiers(fn);
            insert_phi(fn);

            version_map vm;
            stack s;
            rename(fn.entry, vm, s);
        }
    };

    struct cfg2ssa : pass {

        void verify_ssa(const ir::function & fn) {
            // TODO:
        }

        void run(ir::function &fn) override {
            ssa_builder sb{};
            sb.transform_ssa(fn);
            verify_ssa(fn);
        }
    };
}
