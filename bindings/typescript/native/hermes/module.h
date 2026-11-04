#pragma once

#include <functional>
#include <memory>

namespace facebook::jsi {
class Runtime;
}

namespace mln::hermes {

// Posts work from any thread to the runtime thread, never inline. The host
// may discard queued work only after cleanup() has run.
using Schedule = std::function<void(std::function<void()>)>;
class Environment;

// Construct on the JavaScript thread. Clean up or destroy the module before its
// runtime, including when abandoning live work: on the JavaScript thread, or on
// another thread once the JavaScript thread has stopped for good and nothing
// else uses the runtime.
class Module {
 public:
  Module(facebook::jsi::Runtime& runtime, mln::hermes::Schedule schedule);
  ~Module();
  void cleanup();

 private:
  std::unique_ptr<Environment> env_;
};

}  // namespace mln::hermes
