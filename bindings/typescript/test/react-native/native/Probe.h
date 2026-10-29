#pragma once

#include <memory>

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>

namespace mln::react_native {
class Environment;
}

namespace mln::test {

// Runs the actual shim's teardown contract under release-mode runtime reloads.
class Probe {
 public:
  Probe(
    facebook::jsi::Runtime& runtime,
    std::shared_ptr<facebook::react::CallInvoker> invoker
  );
  ~Probe();
  void cleanup();

 private:
  struct State;
  std::shared_ptr<State> state_;
  std::unique_ptr<mln::react_native::Environment> env_;
};

}  // namespace mln::test
