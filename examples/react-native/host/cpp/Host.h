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

// Ends the binding's native work and releases its JSI values. Call it on the
// JavaScript thread before React Native destroys the runtime.
void cleanup();

// Runs cleanup() on the JavaScript thread and waits for it, for a platform hook
// that runs on another thread while the JavaScript thread still runs.
void cleanupOnJavaScriptThread();

}  // namespace mln::example
