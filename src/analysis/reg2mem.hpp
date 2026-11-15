#pragma once
#include "pass.hpp"

namespace dungeon {
struct reg2mem : pass {
  void run(ir::function& fn) override {
  }

  std::string description() override {
    return "Allocate space for variables";
  }
};
}
