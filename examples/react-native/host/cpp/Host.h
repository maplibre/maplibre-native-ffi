#pragma once

#include <memory>

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>

namespace mln::example {

// Installs the binding and the surface registry in a new runtime. Call it on
// the JavaScript thread, before JavaScript imports the binding.
void install(
  facebook::jsi::Runtime& runtime,
  std::shared_ptr<facebook::react::CallInvoker> invoker
);

// Ends the binding's native work and releases its JSI values. Call it before
// React Native destroys the runtime, on the JavaScript thread or once that
// thread has stopped for good.
void cleanup();

}  // namespace mln::example
