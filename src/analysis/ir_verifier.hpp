#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../ir/function.hpp"
#include "../ir/module.hpp"

namespace dungeon {
    namespace verifier_rule {
        inline constexpr const char *entry = "entry"; // fn.entry exists and is one of fn.blocks
        inline constexpr const char *unique_block_id = "unique-block-id";
        inline constexpr const char *edge_target = "edge-target"; // succ/pred point into this function
        inline constexpr const char *edge_dup = "edge-dup"; // no block listed twice in succ or in pred
        inline constexpr const char *edge_symmetry = "edge-symmetry"; // a in b.pred  <=>  b in a.succ
        inline constexpr const char *entry_no_preds = "entry-no-preds";

        inline constexpr const char *parent = "parent"; // instruction.parent is the block holding it
        inline constexpr const char *owned = "owned"; // ... and fn.instructions owns it
        inline constexpr const char *no_label = "no-label"; // labels are gone after cfg_builder
        inline constexpr const char *terminator = "terminator"; // one terminator, last
        inline constexpr const char *succ_count = "succ-count"; // the terminator agrees with succ

        inline constexpr const char *def_link = "def-link"; // result->defining_instruction is the definer
        inline constexpr const char *single_def = "single-def"; // SSA: one definition per value
        inline constexpr const char *operand_null = "operand-null";
        inline constexpr const char *operand_def = "operand-def"; // an operand is defined by a live instruction
        inline constexpr const char *use_list = "use-list"; // value.users matches the real uses

        inline constexpr const char *type = "type";
        inline constexpr const char *signedness = "signedness"; // sdiv on signed, udiv on unsigned, ...
        inline constexpr const char *param_entry = "param-entry";

        inline constexpr const char *phi_first = "phi-first"; // phis come first in their block
        inline constexpr const char *phi_preds = "phi-preds"; // incoming blocks == predecessors
        inline constexpr const char *phi_operands = "phi-operands"; // operands == incoming values
        inline constexpr const char *dominance = "dominance"; // every def dominates its uses
    }

    struct verify_error {
        std::string rule;
        std::string message;
    };

    class ir_verifier {
    public:
        static std::vector<verify_error> verify(const ir::function &fn, const ir::module *mod = nullptr) {
            ir_verifier v{fn, mod};
            v.run();
            return std::move(v.errors_);
        }

        static void verify_or_throw(const ir::function &fn, std::string_view context,
                                    const ir::module *mod = nullptr) {
            const auto errors = verify(fn, mod);
            if (errors.empty())
                return;
            std::string text = "IR verification failed after " + std::string(context) + ":";
            for (const auto &e: errors)
                text += "\n  [" + e.rule + "] " + e.message;
            throw std::runtime_error(text);
        }

    private:
        const ir::function &fn_;
        const ir::module *mod_;
        std::vector<verify_error> errors_;

        std::unordered_set<const basic_block *> blocks_; // every block of fn_
        std::unordered_set<const ir::instruction *> owned_; // everything in fn_.instructions
        std::unordered_set<const ir::instruction *> placed_; // every instruction found inside a block
        std::unordered_map<const ir::instruction *, size_t> position_; // index within its block

        ir_verifier(const ir::function &fn, const ir::module *mod) : fn_(fn), mod_(mod) {
        }

        void fail(const char *rule, std::string message) {
            errors_.push_back({rule, std::move(message)});
        }

        static std::string name(const basic_block *b) {
            return b ? "bb" + std::to_string(b->id.id) : "<null block>";
        }

        static std::string name(const ir::value *v) {
            return v ? "%" + std::to_string(v->id) : "<null value>";
        }

        void build_indexes() {
            for (const auto &b: fn_.blocks)
                blocks_.insert(b.get());
            for (const auto &i: fn_.instructions)
                owned_.insert(i.get());
            for (const auto &b: fn_.blocks) {
                size_t k = 0;
                for (const ir::instruction *i: b->instructions) {
                    if (!i)
                        continue;
                    placed_.insert(i);
                    position_.emplace(i, k++);
                }
            }
        }

        void run() {
            if (!fn_.entry) {
                fail(verifier_rule::entry, "function has no entry block");
                return;
            }
            build_indexes();
            check_entry();
            check_block_ids();
            check_edges();
            check_instruction_placement();
            check_terminators();
            check_definitions();
            check_use_lists();
            check_types();
            compute_dominators();
            check_phis();
            check_dominance();
        }

        void check_entry() {
            bool found = false;
            for (auto &block: blocks_) {
                if (block == fn_.entry)
                    found = true;

                for (const auto succ: block->succ)
                    if (succ == fn_.entry)
                        fail(verifier_rule::entry_no_preds, "branch into entry block");
            }

            if (!found)
                fail(verifier_rule::entry, "not found");
        }

        // RULE unique-block-id
        //   No two blocks share an id. (The printer, phi maps and `edge` all assume ids identify blocks.)
        void check_block_ids() {
            std::unordered_set<uint32_t> seen;
            for (const auto block: blocks_) {
                if (auto [_, was_new] = seen.insert(block->id.id); !was_new)
                    fail(verifier_rule::unique_block_id, "redundant id ");
            }
        }

        void check_edges_helper(const basic_block *source, const std::vector<basic_block *> &targets,
                                bool are_successors) {
            std::unordered_set<basic_block *> seen_targets;
            for (const auto target: targets) {
                if (target == nullptr) {
                    fail(verifier_rule::edge_target, "non-existing target of an edge");
                    continue;
                }
                if (!blocks_.contains(target)) {
                    fail(verifier_rule::edge_target, "edge target does not belong to this function");
                    continue;
                }
                if (auto [_, was_new] = seen_targets.insert(target); !was_new) {
                    fail(verifier_rule::edge_dup, "duplicated edge");
                    continue;
                }
                if (are_successors && std::ranges::find(target->pred, source) == target->pred.end()) {
                    fail(verifier_rule::edge_symmetry, "missing predecessor");
                    continue;
                }
                if (!are_successors && std::ranges::find(target->succ, source) == target->succ.end())
                    fail(verifier_rule::edge_symmetry, "missing successor");
            }
        }

        void check_edges() {
            for (const auto block: blocks_) {
                check_edges_helper(block, block->succ, true);
                check_edges_helper(block, block->pred, false);
            }
        }

        void check_instruction_placement() {
            for (const auto block: blocks_) {
                for (auto ins: block->instructions) {
                    if (ins == nullptr) {
                        fail(verifier_rule::owned, "null instruction encountered");
                        continue;
                    }
                    if (!owned_.contains(ins)) {
                        fail(verifier_rule::owned, "owned does not contain this instruction");
                        continue;
                    }

                    if (ins->parent != block) {
                        fail(verifier_rule::parent, "instruction has a bad parent");
                        continue;
                    }

                    if (ins->op == ir::opcode::label)
                        fail(verifier_rule::no_label, "instruction is a label for some reason");
                }
            }
        }

        void check_terminators() {
            for (const auto block: blocks_) {
                if (block->instructions.empty()) {
                    fail(verifier_rule::terminator, "no terminator");
                    continue;
                }
                for (std::size_t i = 0; i < block->instructions.size(); ++i) {
                    const auto ins = block->instructions[i];
                    if (ins == nullptr)
                        continue;
                    if (size_t succ_size = block->succ.size(); (ins->op == ir::opcode::ret && succ_size != 0) ||
                                                               (ins->op == ir::opcode::br && succ_size != 1) ||
                                                               (ins->op == ir::opcode::cond_br && (
                                                                    succ_size < 1 || succ_size > 2))
                    ) {
                        fail(verifier_rule::succ_count, "invalid amount of successors");
                        continue;
                    }

                    if (i == block->instructions.size() - 1 && !ins->is_terminator()) {
                        fail(verifier_rule::terminator,
                             "expected a terminator as the last instruction in a basic block");
                        continue;
                    }

                    if (i < block->instructions.size() - 1 && ins->is_terminator())
                        fail(verifier_rule::terminator,
                             "encountered a terminator in the middle of a block");
                }
            }
        }

        void check_definitions() {
            std::unordered_set<const ir::value *> defs;

            for (const auto ins: placed_) {
                if (ins->result == nullptr)
                    continue;

                if (auto [_, was_new] = defs.insert(ins->result); !was_new) {
                    fail(verifier_rule::single_def, "redefinition encountered at " + name(ins->result));
                    continue;
                }
                if (ins->result->defining_instruction != ins)
                    fail(verifier_rule::def_link, "invalid definition link at " + name(ins->result));
            }

            for (const auto ins: placed_)
                for (const auto operand: ins->operands) {
                    if (operand == nullptr) {
                        fail(verifier_rule::operand_null, "operand null at " + name(ins->result));
                        continue;
                    }

                    if (!defs.contains(operand))
                        fail(verifier_rule::operand_def, "operand not defined at " + name(ins->result));
                }
        }

        void check_use_lists() {
            using count_map = std::unordered_map<const ir::instruction *, int>;
            std::unordered_map<const ir::value *, count_map> actual;
            std::unordered_set<const ir::value *> values;

            for (const auto ins: placed_) {
                if (ins->result != nullptr)
                    values.insert(ins->result);

                for (const auto operand: ins->operands) {
                    if (operand != nullptr) {
                        ++actual[operand][ins];
                        values.insert(operand);
                    }
                }
            }

            for (auto val: values) {
                std::unordered_map<const ir::instruction *, int> listed;
                for (auto user: val->users)
                    listed[user]++;

                auto &acc = actual[val];
                for (auto &[ins, acc_count]: acc) {
                    if (!listed.contains(ins) || listed[ins] != acc_count) {
                        fail(verifier_rule::use_list, "uses/users list mismatch");
                        break;
                    }
                }

                for (auto &[ins, lis_count]: listed) {
                    if (!acc.contains(ins) || acc[ins] != lis_count) {
                        fail(verifier_rule::use_list, "uses/users list mismatch");
                        break;
                    }
                }
            }
        }

        // =====================================================================================
        // PART 4 - types
        // =====================================================================================

        // RULES type, signedness, param-entry
        //   Look at what the lowering produces (print some IR) and write down the typing rule of
        //   every opcode as you go. Suggested rules (adjust them if the IR says otherwise):
        //     iconst            no operands; result is an integer type
        //     bconst            no operands; result is bool
        //     add sub mul       2 operands and the result all have ONE integer type
        //     shl shr sar       same (the shift amount has the operand's type)
        //     udiv umod sdiv smod   same
        //     neg               1 operand, same integer type as the result
        //     lnot              bool -> bool
        //     eq                2 operands of the same type; result bool
        //     slt sle sgt sge, ult ule ugt uge
        //                       2 operands of the same INTEGER type; result bool
        //     cond_br           exactly one operand, of type bool
        //     alloca            a result, no operands
        //     load              1 operand: a slot (an alloca or a param); result type == slot type
        //     store             2 operands {slot, value}, NO result; value type == slot type
        //     ret               no result if fn.return_type is unit: no operand, else 1 operand of
        //                       type fn.return_type
        //     param             result type == fn.param_types[index], and it lives in the entry block
        //     phi               every operand has the phi's result type
        //   And the one rule that is easy to forget -- the opcode and the type must agree about
        //   signedness (see ir/int_semantics.hpp): udiv, umod, shr and ult/ule/ugt/uge need an
        //   unsigned operand type; sdiv, smod, sar and slt/sle/sgt/sge need a signed one. Report
        //   those as `signedness`, not `type`, and param outside the entry as `param-entry`.
        //

        bool need_result(const ir::instruction *ins) {
            if (ins->result != nullptr)
                return true;
            fail(verifier_rule::type, "missing result");
            return false;
        }

        bool check_operands(const ir::instruction *ins, size_t expected) {
            if (ins->operands.size() != expected) {
                fail(verifier_rule::type, "invalid amount of operands at " + name(ins->result));
                return false;
            }

            for (size_t i = 0; i < expected; ++i)
                if (ins->operands[i] == nullptr)
                    return false;

            return true;
        }

        bool check_binary_num_op(const ir::instruction *ins) {
            if (!check_operands(ins, 2))
                return false;

            if (!need_result(ins))
                return false;

            if (!is_integer_ty(ins->operands[0]->ty) || !is_integer_ty(ins->operands[1]->ty)) {
                fail(verifier_rule::type, "expected operands of integer type " + name(ins->result));
                return false;
            }

            if (ins->operands[0]->ty != ins->operands[1]->ty) {
                fail(verifier_rule::type, "expected operands of the same type " + name(ins->result));
                return false;
            }

            if (ins->operands[0]->ty != ins->result->ty) {
                fail(verifier_rule::type, "invalid result type " + name(ins->result));
                return false;
            }

            return true;
        }

        bool check_binary_rel_op(const ir::instruction *ins) {
            if (!check_operands(ins, 2)) {
                return false;
            }

            if (!need_result(ins))
                return false;

            if (ins->operands[0]->ty != ins->operands[1]->ty) {
                fail(verifier_rule::type, "expected operands of the same type " + name(ins->result));
                return false;
            }

            if (!is_boolean_ty(ins->result->ty)) {
                fail(verifier_rule::type, "expected boolean type of result " + name(ins->result));
                return false;
            }
            return true;
        }

        void check_unary_log_op(const ir::instruction *ins) {
            if (!check_operands(ins, 1))
                return;

            if (!need_result(ins))
                return;

            if (!is_boolean_ty(ins->operands[0]->ty))
                fail(verifier_rule::type, "expected a boolean operand " + name(ins->result));

            if (!is_boolean_ty(ins->result->ty))
                fail(verifier_rule::type, "expected boolean type of result " + name(ins->result));
        }

        void check_br(const ir::instruction *ins, bool cond) {
            if (!check_operands(ins, cond ? 1 : 0))
                return;

            if (cond && !is_boolean_ty(ins->operands[0]->ty))
                fail(verifier_rule::type, "expected boolean type of operand " + name(ins->result));

            if (ins->result != nullptr)
                fail(verifier_rule::type, "unexpected result of an operation " + name(ins->result));
        }

        void check_ret(const ir::instruction *ins) {
            if (ins->result) {
                fail(verifier_rule::type, "invalid return type " + name(ins->result));
                return;
            }

            if (is_unit_ty(fn_.return_type)) {
                if (!ins->operands.empty())
                    fail(verifier_rule::type, "unexpected return in a unit function " + name(ins->result));
                return;
            }

            if (!check_operands(ins, 1))
                return;

            if (!same_type(ins->operands[0]->ty, fn_.return_type))
                fail(verifier_rule::type, "unexpected return type " + name(ins->result));
        }

        void check_unary_num_op(const ir::instruction *ins) {
            if (!check_operands(ins, 1))
                return;

            if (!need_result(ins))
                return;

            if (!is_integer_ty(ins->operands[0]->ty))
                fail(verifier_rule::type, "expected operand of integer type " + name(ins->result));

            if (ins->operands[0]->ty != ins->result->ty)
                fail(verifier_rule::type, "invalid result type " + name(ins->result));
        }

        void check_binary_unsigned_num_op(const ir::instruction *ins) {
            if (!check_binary_num_op(ins))
                return;

            if (!is_unsigned_integer_ty(ins->operands[0]->ty) || !is_unsigned_integer_ty(ins->operands[1]->ty))
                fail(verifier_rule::signedness, "expected unsigned operands " + name(ins->result));
        }

        void check_binary_signed_num_op(const ir::instruction *ins) {
            if (!check_binary_num_op(ins))
                return;

            if (!is_signed_integer_ty(ins->operands[0]->ty) || !is_signed_integer_ty(ins->operands[1]->ty))
                fail(verifier_rule::signedness, "expected signed operands " + name(ins->result));
        }


        void check_binary_signed_rel_op(const ir::instruction *ins) {
            if (!check_binary_rel_op(ins))
                return;

            if (!is_signed_integer_ty(ins->operands[0]->ty) || !is_signed_integer_ty(ins->operands[1]->ty))
                fail(verifier_rule::signedness, "expected signed operands " + name(ins->result));
        }

        void check_binary_unsigned_rel_op(const ir::instruction *ins) {
            if (!check_binary_rel_op(ins))
                return;

            if (!is_unsigned_integer_ty(ins->operands[0]->ty) || !is_unsigned_integer_ty(ins->operands[1]->ty))
                fail(verifier_rule::signedness, "expected unsigned operands " + name(ins->result));
        }


        static bool is_slot(const ir::value *v) {
            if (v == nullptr || v->defining_instruction == nullptr)
                return false;
            const auto op = v->defining_instruction->op;
            return op == ir::opcode::alloca || op == ir::opcode::param;
        }

        void check_types() {
            for (const auto ins: placed_) {
                switch (ins->op) {
                    case ir::opcode::iconst: {
                        if (!check_operands(ins, 0) || !need_result(ins))
                            break;

                        if (!is_integer_ty(ins->result->ty))
                            fail(verifier_rule::type, "expected integer return type for iconst " + name(ins->result));
                        break;
                    }
                    case ir::opcode::bconst: {
                        if (!check_operands(ins, 0) || !need_result(ins))
                            break;

                        if (!is_boolean_ty(ins->result->ty))
                            fail(verifier_rule::type, "expected boolean type for bconst " + name(ins->result));
                        break;
                    }
                    case ir::opcode::add:
                    case ir::opcode::sub:
                    case ir::opcode::mul:
                    case ir::opcode::shl:
                        check_binary_num_op(ins);
                        break;

                    case ir::opcode::shr:
                        check_binary_unsigned_num_op(ins);
                        break;

                    case ir::opcode::sar:
                        check_binary_signed_num_op(ins);
                        break;

                    case ir::opcode::neg:
                        check_unary_num_op(ins);
                        break;

                    case ir::opcode::umod:
                    case ir::opcode::udiv:
                        check_binary_unsigned_num_op(ins);
                        break;

                    case ir::opcode::smod:
                    case ir::opcode::sdiv:
                        check_binary_signed_num_op(ins);
                        break;

                    case ir::opcode::slt:
                    case ir::opcode::sgt:
                    case ir::opcode::sle:
                    case ir::opcode::sge:
                        check_binary_signed_rel_op(ins);
                        break;

                    case ir::opcode::ult:
                    case ir::opcode::ugt:
                    case ir::opcode::ule:
                    case ir::opcode::uge:
                        check_binary_unsigned_rel_op(ins);
                        break;

                    case ir::opcode::eq:
                        check_binary_rel_op(ins);
                        break;

                    case ir::opcode::lnot:
                        check_unary_log_op(ins);
                        break;

                    case ir::opcode::br:
                        check_br(ins, false);
                        break;

                    case ir::opcode::cond_br:
                        check_br(ins, true);
                        break;

                    case ir::opcode::alloca: {
                        if (!check_operands(ins, 0))
                            break;

                        if (ins->result == nullptr)
                            fail(verifier_rule::type, "missing result of an instruction " + name(ins->result));

                        break;
                    }
                    case ir::opcode::load: {
                        if (!check_operands(ins, 1) || !need_result(ins))
                            break;

                        if (!same_type(ins->result->ty, ins->operands[0]->ty))
                            fail(verifier_rule::type,
                                 "expected result to have the same type as operand " + name(ins->result));

                        if (!is_slot(ins->operands[0]))
                            fail(verifier_rule::type, "load operand must be a slot " + name(ins->result));
                        break;
                    }
                    case ir::opcode::store: {
                        if (!check_operands(ins, 2))
                            break;

                        if (ins->result != nullptr)
                            fail(verifier_rule::type, "instruction expected to have no return " + name(ins->result));

                        if (!same_type(ins->operands[0]->ty, ins->operands[1]->ty))
                            fail(verifier_rule::type, "expected operands to have the same type " + name(ins->result));

                        if (!is_slot(ins->operands[0]))
                            fail(verifier_rule::type, "first store operand must be a slot " + name(ins->result));

                        break;
                    }
                    case ir::opcode::call:
                        break;
                    case ir::opcode::ret: {
                        check_ret(ins);
                        break;
                    }
                    case ir::opcode::param: {
                        if (!need_result(ins))
                            break;

                        auto pd = std::get_if<ir::param_data>(&ins->data);
                        if (!pd) {
                            fail(verifier_rule::type,
                                 "expected instruction to have parameter data " + name(ins->result));
                            break;
                        }

                        if (pd->index >= fn_.param_types.size() || !same_type(
                                fn_.param_types[pd->index], ins->result->ty)) {
                            fail(verifier_rule::type,
                                 "mismatching function signature param type and actual type provided in instruction " +
                                 name(ins->result));
                            break;
                        }

                        if (ins->parent != fn_.entry)
                            fail(verifier_rule::param_entry,
                                 "parameter must be in the entry block " + name(ins->result));
                        break;
                    }
                    case ir::opcode::phi:
                        if (!need_result(ins))
                            break;
                        for (const auto oper: ins->operands)
                            if (oper != nullptr && !same_type(ins->result->ty, oper->ty))
                                fail(verifier_rule::type,
                                     "expected operands to have the same type " + name(ins->result));
                        break;
                    case ir::opcode::label:
                        break;
                }
            }
        }

        // =====================================================================================
        // PART 5 - dominance and phis
        // =====================================================================================
        //
        // The rule that makes SSA meaningful: every use of a value must be DOMINATED by its
        // definition. Block A dominates block B if every path from the entry to B goes through
        // A. If a use isn't dominated, there is a path on which the value was never computed.
        //
        // You can't trust basic_block::idom / df here: they are computed by mem2reg and go stale
        // when later passes change the CFG. Compute dominators yourself, from scratch, on every
        // call -- that is what makes the verifier independent of the passes it checks.

        // Dominator data. Add whatever members you need (I used a reverse-postorder vector, a
        // block -> RPO-index map and an idom array indexed by RPO number).
        // TODO: implement this (the comment above says what to check).

        // Compute the dominator tree of the blocks reachable from fn_.entry using the iterative
        // algorithm of Cooper, Harvey and Kennedy ("A Simple, Fast Dominance Algorithm"):
        //   1. number the reachable blocks in reverse postorder (DFS from the entry);
        //   2. idom[entry] = entry; repeat until nothing changes: for each other block b in RPO,
        //      new_idom = intersection (walking up the idom chains, by RPO number) over its
        //      already-processed predecessors.
        // Unreachable blocks are not in the tree -- they have no dominators and are skipped by the
        // checks below (code that can never run has no execution path to violate).
        //
        // HINT: write the DFS iteratively or recursively -- either way, only follow succ pointers
        //   you have already validated against blocks_.
        void compute_dominators() {
            // TODO: implement this (the comment above says what to check).
        }

        // Is `b` reachable from the entry?
        bool reachable(const basic_block *b) const {
            // TODO: implement this (the comment above says what to check).
            return false;
        }

        // Does `a` dominate `b`? (Every block dominates itself.) Both must be reachable.
        // Walk up the idom chain from b until you meet a or the entry.
        bool dominates(const basic_block *a, const basic_block *b) const {
            // TODO: implement this (the comment above says what to check).
            return false;
        }

        // RULES phi-first, phi-preds, phi-operands
        //   A phi picks a value depending on which predecessor control came from. This IR stores a
        //   phi twice: `operands` (what use-lists and replace_all_uses_with see) and
        //   `phi_data::incoming` (block -> value, what SCCP and simplify_cfg look at). Both views
        //   must describe the same thing.
        //   * phi-first:    in a block, all phis come before every non-phi instruction;
        //   * phi-preds:    the SET of blocks in `incoming` equals the SET of the block's
        //                   predecessors exactly -- a missing entry means a path with no value, an
        //                   extra one is an edge that doesn't exist;
        //   * phi-operands: the values in `incoming` equal `operands` (compare as multisets).
        //   (The phi's type was already checked in Part 4. A phi with no phi_data is reported as
        //   `phi-operands`.)
        void check_phis() {
            // TODO: implement this (the comment above says what to check).
        }

        // RULE dominance
        //   For every instruction I in a REACHABLE block B, and every operand v with definition D:
        //     * ordinary use:   D must come before I -- earlier in the same block, or in a block
        //                       that dominates B.
        //     * phi use (v flows in from predecessor P): the use happens at the END of P, so D must
        //                       dominate P. (It does NOT have to dominate the phi's own block --
        //                       that's what makes loops work: the back-edge value is defined in the
        //                       loop body, which does not dominate the loop header.)
        //   Skip operands whose definition is missing or unreachable; Part 3 already reported them.
        //
        // HINT: position_ gives the index of an instruction within its block.
        // PITFALL: a phi at the top of a loop header uses a value defined LATER in the loop. If your
        //   check complains about a correct loop, you're treating the phi use as an ordinary use.
        void check_dominance() {
            // TODO: implement this (the comment above says what to check).
        }
    };
}
