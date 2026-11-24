#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "pass.hpp"
#include "ir_verifier.hpp"
#include "../common/pretty_printer.hpp"

namespace dungeon::analysis {
struct pass_manager {

  std::vector<std::unique_ptr<pass>> owned;
  std::vector<pass*> schedule;
  std::function<void(pass&, ir::function&)> after_pass;

  pass* add(std::unique_ptr<pass> pass) {
    owned.push_back(std::move(pass));
    auto raw = owned.back().get();
    schedule.push_back(raw);
    return raw;
  }

  void repeat(pass* p) {
    schedule.push_back(p);
  }

  void run(ir::function& fn, bool verify = false) {
    for (auto pass : schedule) {
      pass->run(fn);
      if (verify && pass->preserves_ssa())
        ir_verifier::verify_or_throw(fn, pass->description());
      if (after_pass)
        after_pass(*pass, fn);
    }
  }
};

}
