// Tests for sequentialize_parallel_copy (src/analysis/parallel_copy.hpp).
//
// A parallel copy {d1 <- s1, ..., dn <- sn} means: read every source, then write every
// destination. The sequential list returned by the function must leave every *original* location
// with exactly the value the parallel copy would have given it.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../../src/analysis/parallel_copy.hpp"

using dungeon::move;
using dungeon::sequentialize_parallel_copy;

static int failures = 0;

#define CHECK(cond, ...)                                                                   \
  do {                                                                                     \
    if (!(cond)) {                                                                         \
      ++failures;                                                                          \
      std::printf("FAIL %s:%d: %s -- ", __FILE__, __LINE__, #cond);                        \
      std::printf(__VA_ARGS__);                                                            \
      std::printf("\n");                                                                   \
    }                                                                                      \
  } while (0)

static std::string show(const std::vector<move>& ms) {
  std::string s;
  for (const move& m : ms)
    s += " " + std::to_string(m.dst) + "<-" + std::to_string(m.src);
  return s;
}

struct outcome {
  std::vector<move> seq;
  uint32_t temps = 0;
  bool ok = true;
};

// Runs the function and checks it against the parallel semantics. `locations` are the original
// locations 0..locations-1; temporaries are numbered from `locations` upwards.
static outcome run(uint32_t locations, const std::vector<move>& parallel) {
  outcome r;
  uint32_t next = locations;
  r.seq = sequentialize_parallel_copy(parallel, [&](uint32_t) {
    ++r.temps;
    return next++;
  });

  std::map<uint32_t, int> before;
  for (uint32_t l = 0; l < locations; ++l)
    before[l] = 1000 + static_cast<int>(l);

  std::map<uint32_t, int> expected = before;
  for (const move& m : parallel)
    expected[m.dst] = before.at(m.src);

  std::map<uint32_t, int> env = before;
  for (const move& m : r.seq) {
    if (!env.contains(m.src)) {
      std::printf("  reads %u before anything wrote it:%s\n", m.src, show(r.seq).c_str());
      r.ok = false;
      return r;
    }
    env[m.dst] = env.at(m.src);
  }
  for (uint32_t l = 0; l < locations; ++l)
    if (env.at(l) != expected.at(l)) {
      std::printf("  location %u holds %d, expected %d for input%s -> output%s\n", l, env.at(l),
                  expected.at(l), show(parallel).c_str(), show(r.seq).c_str());
      r.ok = false;
      return r;
    }
  return r;
}

int main() {
  {  // nothing to do
    const auto r = run(3, {});
    CHECK(r.ok && r.seq.empty(), "an empty parallel copy is an empty sequence");
  }
  {  // self copies vanish
    const auto r = run(3, {{1, 1}, {2, 2}});
    CHECK(r.ok && r.seq.empty(), "x <- x must not be emitted:%s", show(r.seq).c_str());
  }
  {  // independent moves need no temporary
    const auto r = run(4, {{0, 2}, {1, 3}});
    CHECK(r.ok && r.temps == 0 && r.seq.size() == 2, "independent moves:%s", show(r.seq).c_str());
  }
  {  // a chain must be emitted from the end: 0<-1, 1<-2 means 0 gets the OLD 1
    const auto r = run(3, {{0, 1}, {1, 2}});
    CHECK(r.ok && r.temps == 0 && r.seq.size() == 2, "chain needs no temp:%s",
          show(r.seq).c_str());
  }
  {  // fan-out: one source, several destinations
    const auto r = run(4, {{0, 3}, {1, 3}, {2, 3}});
    CHECK(r.ok && r.temps == 0 && r.seq.size() == 3, "fan-out:%s", show(r.seq).c_str());
  }
  {  // the swap: the classic
    const auto r = run(2, {{0, 1}, {1, 0}});
    CHECK(r.ok && r.temps == 1 && r.seq.size() == 3, "swap needs exactly one temp, 3 moves:%s",
          show(r.seq).c_str());
  }
  {  // rotation of three
    const auto r = run(3, {{0, 1}, {1, 2}, {2, 0}});
    CHECK(r.ok && r.temps == 1 && r.seq.size() == 4, "3-cycle needs one temp, 4 moves:%s",
          show(r.seq).c_str());
  }
  {  // a cycle with a tail hanging off it
    const auto r = run(4, {{0, 1}, {1, 0}, {2, 1}, {3, 2}});
    CHECK(r.ok, "cycle with tail");
  }
  {  // two independent cycles
    const auto r = run(4, {{0, 1}, {1, 0}, {2, 3}, {3, 2}});
    CHECK(r.ok && r.temps == 2, "two swaps need two temps (or reuse one):%s",
          show(r.seq).c_str());
  }
  {  // a cycle that is also read by a move outside it
    const auto r = run(4, {{0, 1}, {1, 0}, {2, 0}});
    CHECK(r.ok, "cycle read from outside");
  }

  // Random parallel copies: every destination at most once, sources anywhere.
  std::mt19937 rng(12345);
  for (int round = 0; round < 20000; ++round) {
    const uint32_t locations = 1 + rng() % 8;
    std::vector<uint32_t> dsts(locations);
    for (uint32_t i = 0; i < locations; ++i)
      dsts[i] = i;
    std::shuffle(dsts.begin(), dsts.end(), rng);
    const uint32_t n = rng() % (locations + 1);
    std::vector<move> parallel;
    for (uint32_t i = 0; i < n; ++i)
      parallel.push_back({dsts[i], static_cast<uint32_t>(rng() % locations)});
    const auto r = run(locations, parallel);
    CHECK(r.ok, "random round %d", round);
    CHECK(r.temps <= locations / 2, "round %d used %u temporaries for %u locations", round,
          r.temps, locations);
    if (failures > 10)
      break;
  }

  if (failures == 0)
    std::printf("parallel_copy_test: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
