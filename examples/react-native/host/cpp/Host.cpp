#include <cstdio>
#include <mutex>

#include <module.h>

#include "Host.h"

#include "Surfaces.h"

namespace jsi = facebook::jsi;
using facebook::react::CallInvoker;

namespace mln::example {

namespace {

// React Native runs one runtime at a time. A reload destroys the old one after
// cleanup() and installs the module in the new one.
std::mutex mutex;
std::unique_ptr<mln::hermes::Module> currentModule;

}  // namespace

void install(jsi::Runtime& runtime, std::shared_ptr<CallInvoker> invoker) {
  {
    std::lock_guard lock(mutex);
    if (currentModule) {
      // The old runtime went away without cleanup(), so its module can't be
      // destroyed safely. Leak it rather than crash.
      std::fprintf(
        stderr, "MaplibreHost: the previous runtime skipped cleanup\n"
      );
      currentModule.release();
    }
  }
  // The binding requires a scheduler that never runs work inline. React
  // Native's invoker queues the work on the JavaScript thread.
  auto module = std::make_unique<mln::hermes::Module>(
    runtime, [invoker](std::function<void()> task) {
      invoker->invokeAsync([task = std::move(task)](jsi::Runtime&) { task(); });
    }
  );
  surfaces::install(runtime);
  std::lock_guard lock(mutex);
  currentModule = std::move(module);
}

void cleanup() {
  std::unique_ptr<mln::hermes::Module> module;
  {
    std::lock_guard lock(mutex);
    module = std::move(currentModule);
  }
  if (module) {
    module->cleanup();
    module.reset();
  }
  // Cleanup ended every session, so JavaScript no longer holds a surface.
  surfaces::releaseAll();
}

}  // namespace mln::example
