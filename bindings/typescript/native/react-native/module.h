#pragma once

#include "shim.h"

namespace mln::react_native {

class Module {
 public:
  Module(
    facebook::jsi::Runtime& runtime,
    std::shared_ptr<facebook::react::CallInvoker> invoker
  );
  void cleanup();

 private:
  Environment env_;
};

}  // namespace mln::react_native
