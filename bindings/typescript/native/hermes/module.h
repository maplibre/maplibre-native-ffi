#pragma once

#include <functional>
#include <memory>

namespace facebook::jsi {
class Runtime;
}

namespace mln::hermes {

// Posts work from any thread to the runtime thread, never inline. The host
// may discard queued work only after cleanup() has run on that thread.
using Schedule = std::function<void(std::function<void()>)>;
class Environment;

// Construct and destroy on the JavaScript thread. The module must be cleaned
// up or destroyed before its runtime, including when abandoning live work.
class Module {
 public:
  Module(facebook::jsi::Runtime& runtime, mln::hermes::Schedule schedule);
  ~Module();
  void cleanup();

 private:
  std::unique_ptr<Environment> env_;
};

}  // namespace mln::hermes
