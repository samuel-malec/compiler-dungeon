#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../sema/symbol_table.hpp"
#include "int_semantics.hpp"
#include "module.hpp"

namespace dungeon::ir {
enum class outcome {
  returned, // the function returned normally
  trap, // a runtime error the language defines
  out_of_fuel, // the step limit ran out
  error, // the IR itself is broken, e.g. a compiler bug
};

struct exec_result {
  outcome kind = outcome::error;
  uint64_t value = 0;
  const type* ty = nullptr; // the function's return type, when `returned`
  std::string message; // the trap reason / error description

  bool operator==(const exec_result& o) const {
    if (kind != o.kind)
      return false;
    if (kind == outcome::returned)
      return value == o.value;
    if (kind == outcome::trap)
      return message == o.message;
    return true;
  }
};

inline std::string to_string(const exec_result& r) {
  switch (r.kind) {
  case outcome::returned:
    if (r.ty == nullptr || is_unit_ty(r.ty))
      return "result: ()";
    if (is_boolean_ty(r.ty))
      return std::string("result: ") + (r.value ? "true" : "false");
    if (is_signed_integer_ty(r.ty))
      return "result: " + std::to_string(static_cast<int64_t>(r.value));
    return "result: " + std::to_string(r.value);
  case outcome::trap:
    return "trap: " + r.message;
  case outcome::out_of_fuel:
    return "out of fuel";
  case outcome::error:
    return "error: " + r.message;
  }
  return "?";
}

inline std::optional<size_t> find_entry(const module& mod,
                                        const sema::analysis_result& sema) {
  for (const auto& sym : sema.symbols)
    if (const auto* fn = std::get_if<sema::function>(&sym.data);
      fn && sema.names.at(sym.nid.value) == "main" && fn->id.value < mod.funcs.
      size())
      return fn->id.value;
  for (size_t i = 0; i < mod.funcs.size(); ++i)
    if (mod.funcs[i].param_types.empty())
      return i;
  return std::nullopt;
}

class interpreter {
public:
  explicit interpreter(const module& mod,
                       uint64_t fuel = 1'000'000,
                       unsigned max_depth = 1000)
    : mod_(mod)
    , fuel_(fuel)
    , max_depth_(max_depth) {
  }

  exec_result run(const sema::analysis_result& sema, const std::vector<uint64_t>& args = {}) {
    auto res = find_entry(mod_, sema);
    if (!res) {
      std::string msg = "could not find 'main' entry point";
      diag::error(msg);
      return {.kind = outcome::error, .message = msg};
    }
    return do_run(*res, args);
  }

  exec_result do_run(size_t fn_index, const std::vector<uint64_t>& args = {}) {
    steps_ = 0;
    try {
      const uint64_t bits = call(fn_index, args, 0);
      return {outcome::returned, bits, mod_.funcs.at(fn_index).return_type, {}};
    } catch (const fault& f) {
      return {f.kind, 0, nullptr, f.message};
    }
  }

private:
  struct fault {
    outcome kind;
    std::string message;
  };

  using registers = std::unordered_map<const value*, uint64_t>;

  const module& mod_;
  uint64_t fuel_;
  unsigned max_depth_;
  uint64_t steps_ = 0;

  [[noreturn]] static void trap(std::string why) {
    throw fault{outcome::trap, std::move(why)};
  }

  [[noreturn]] static void broken(std::string why) {
    throw fault{outcome::error, std::move(why)};
  }

  void tick() {
    if (++steps_ > fuel_)
      throw fault{.kind = outcome::out_of_fuel,
                  .message = "step limit exceeded"};
  }

  static uint64_t read(const registers& regs, const value* v) {
    if (v == nullptr)
      broken("null operand");
    const auto it = regs.find(v);
    if (it == regs.end())
      broken(
          "use of a value that has no runtime value yet (%" +
          std::to_string(v->id) + ")");
    return it->second;
  }

  static bool is_pure(const instruction* ins) {
    switch (ins->op) {
    case opcode::iconst:
    case opcode::bconst:
    case opcode::add:
    case opcode::sub:
    case opcode::mul:
    case opcode::shl:
    case opcode::shr:
    case opcode::sar:
    case opcode::neg:
    case opcode::umod:
    case opcode::udiv:
    case opcode::smod:
    case opcode::sdiv:
    case opcode::slt:
    case opcode::sgt:
    case opcode::sle:
    case opcode::sge:
    case opcode::ult:
    case opcode::ugt:
    case opcode::ule:
    case opcode::uge:
    case opcode::eq:
    case opcode::lnot:
      return true;
    default:
      return false;
    }
  }

  static bool is_memory(const instruction* ins) {
    switch (ins->op) {
    case opcode::alloca:
    case opcode::load:
    case opcode::store:
    case opcode::param:
      return true;
    default:
      return false;
    }
  }

  void exec_pure(const instruction* ins, registers& regs) {
    tick();
    auto ins_res = ins->result;
    if (ins->op == opcode::iconst) {
      auto id = std::get<iconst_data>(ins->data);
      regs[ins_res] = id.value;
    } else if (ins->op == opcode::bconst) {
      auto bd = std::get<bconst_data>(ins->data);
      regs[ins_res] = bd.value ? 1u : 0u;
    } else if (ins->op == opcode::add || ins->op == opcode::sub || ins->op == opcode::mul ||
               ins->op == opcode::shl || ins->op == opcode::shr || ins->op == opcode::sar ||
               ins->op == opcode::udiv || ins->op == opcode::umod || ins->op == opcode::sdiv
               || ins->op == opcode::smod) {
      const uint64_t lhs = read(regs, ins->operands[0]);
      const uint64_t rhs = read(regs, ins->operands[1]);
      auto res = eval_arith(ins->op, ins->result->ty, lhs, rhs);
      if (!res) {
        // eval_arith has no result exactly in these three cases (see int_semantics.hpp)
        if (ins->op == opcode::shl || ins->op == opcode::shr || ins->op == opcode::sar)
          trap("shift amount out of range");
        trap(rhs == 0 ? "division by zero" : "signed division overflow");
      }
      regs[ins_res] = *res;
    } else if (ins->op == opcode::eq || ins->op == opcode::slt || ins->op ==
               opcode::sle ||
               ins->op == opcode::sgt || ins->op == opcode::sge || ins->op ==
               opcode::ult ||
               ins->op == opcode::ule || ins->op == opcode::ugt || ins->op ==
               opcode::uge) {
      bool cmp = eval_cmp(ins->op,
                          read(regs, ins->operands[0]),
                          read(regs, ins->operands[1]));
      regs[ins_res] = cmp ? 1u : 0u;

    } else if (ins->op == opcode::neg)
      regs[ins_res] = normalize(0 - read(regs, ins->operands[0]), ins->result->ty);

    else if (ins->op == opcode::lnot)
      regs[ins_res] = read(regs, ins->operands[0]) == 0u ? 1u : 0u;

    else
      broken("unexpected pure instruction");

  }

  void exec_memory(const instruction* ins,
                   registers& regs,
                   const std::vector<uint64_t>& args) {
    tick();
    if (ins->op == opcode::alloca)
      regs.erase(ins->result);

    else if (ins->op == opcode::store)
      regs[ins->operands[0]] = read(regs, ins->operands[1]);

    else if (ins->op == opcode::load) {
      auto it = regs.find(ins->operands[0]);

      if (it == regs.end())
        broken("load from the uninitialised slot %" + std::to_string(ins->operands[0]->id));
      regs[ins->result] = it->second;

    } else if (ins->op == opcode::param) {
      const auto pd = std::get_if<param_data>(&ins->data);
      if (pd == nullptr)
        broken("param instruction without param_data");
      if (pd->index >= args.size())
        broken("missing argument for parameter " + std::to_string(pd->index));
      regs[ins->result] = args[pd->index];
    } else
      broken("unexpected instruction");
  }

  std::size_t enter_block(const basic_block* cur,
                          const basic_block* prev,
                          registers& regs) {
    std::size_t idx = 0;
    std::vector<uint64_t> phi_results;

    while (idx < cur->instructions.size() && cur->instructions[idx]->op ==
           opcode::phi) {
      tick();
      auto pd = std::get_if<phi_data>(&cur->instructions[idx]->data);
      if (!pd)
        broken("phi without phi_data");

      auto it = pd->incoming.find(const_cast<basic_block*>(prev));
      if (it == pd->incoming.end())
        broken("phi %" + std::to_string(cur->instructions[idx]->result->id) + " has no incoming value from " +
               (prev ? "bb" + std::to_string(prev->id.id) : std::string("the function entry")));
      phi_results.push_back(read(regs, it->second));
      ++idx;
    }

    for (std::size_t i = 0; i < idx; ++i)
      regs[cur->instructions[i]->result] = phi_results[i];

    return idx;
  }

  uint64_t call(size_t fn_index,
                const std::vector<uint64_t>& args,
                unsigned depth) {
    if (depth > max_depth_)
      trap("stack overflow");

    if (fn_index >= mod_.funcs.size())
      broken("invalid function index");

    const auto& fn = mod_.funcs[fn_index];
    if (fn.entry == nullptr)
      broken("entry block is nullptr");

    registers regs{};
    basic_block* prev = nullptr;
    basic_block* curr = fn.entry;
    std::size_t pc = 0;

    // An `error` outcome is a compiler bug: say where it happened. Nested calls add one frame each,
    // so the message ends up as a small call stack: "... [fn #2, bb3] [fn #0, bb1]".
    // (Traps are not decorated: the tests compare their text exactly.)
    try {
      pc = enter_block(curr, prev, regs);

      while (true) {
        if (pc >= curr->instructions.size())
          broken("instruction counter overflow");

        const auto ins = curr->instructions[pc];
        if (is_pure(ins)) {
          exec_pure(curr->instructions[pc], regs);
          ++pc;
        } else if (is_memory(ins)) {
          exec_memory(ins, regs, args);
          ++pc;
        } else if (ins->op == opcode::br) {
          tick();

          if (curr->succ.size() != 1)
            broken("unexpected amount of block successors, expected 1");

          const auto tmp = curr;
          curr = curr->succ[0];
          prev = tmp;
          pc = enter_block(curr, prev, regs);
        } else if (ins->op == opcode::cond_br) {
          tick();

          // a cond_br whose two arms are the same block has a single successor
          if (curr->succ.empty() || curr->succ.size() > 2)
            broken("cond_br with " + std::to_string(curr->succ.size()) + " successors");

          const auto tmp = curr;
          if (read(regs, ins->operands.at(0)) != 0)
            curr = curr->succ.front();
          else
            curr = curr->succ.back();

          prev = tmp;
          pc = enter_block(curr, prev, regs);
        } else if (ins->op == opcode::call) {
          tick();
          auto cd = std::get_if<call_data>(&ins->data);
          if (!cd)
            broken("invalid call data");

          std::vector<uint64_t> call_args;
          call_args.reserve(ins->operands.size());
          for (auto op : ins->operands)
            call_args.push_back(read(regs, op));

          auto callee_id = cd->target.value;
          const uint64_t returned = call(callee_id, call_args, depth + 1);
          if (ins->result != nullptr)
            regs[ins->result] = returned;
          ++pc;
        } else if (ins->op == opcode::ret) {
          tick();
          return ins->operands.empty() ? 0 : read(regs, ins->operands[0]);
        } else
          broken("invalid instruction encountered");
      }
    } catch (fault& f) {
      if (f.kind == outcome::error)
        f.message += " [fn #" + std::to_string(fn_index) + ", bb" + std::to_string(curr->id.id) + "]";
      throw;
    }
  }
};
} // namespace dungeon::ir
