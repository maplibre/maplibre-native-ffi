#pragma once

#include <memory>

#include <jsi/jsi.h>

#include "shim.h"

namespace mln::test {

// Runs the actual shim's teardown contract under release-mode runtime reloads.
class Probe {
 public:
  Probe(facebook::jsi::Runtime& runtime, mln::hermes::Schedule schedule);
  ~Probe();
  void cleanup();
  static unsigned released();

 private:
  struct State;
  std::shared_ptr<State> state_;
  std::unique_ptr<mln::hermes::Environment> env_;
};

}  // namespace mln::test
