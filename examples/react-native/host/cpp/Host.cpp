#include <chrono>
#include <cstdio>
#include <future>
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
std::shared_ptr<CallInvoker> currentInvoker;
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
    currentInvoker = invoker;
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
    currentInvoker.reset();
  }
  if (module) {
    module->cleanup();
    module.reset();
  }
  // Cleanup ended every session, so JavaScript no longer holds a surface.
  surfaces::releaseAll();
}

void cleanupOnJavaScriptThread() {
  std::shared_ptr<CallInvoker> invoker;
  {
    std::lock_guard lock(mutex);
    invoker = currentInvoker;
  }
  if (!invoker) return;
  auto done = std::make_shared<std::promise<void>>();
  auto finished = done->get_future();
  invoker->invokeAsync([done](jsi::Runtime&) {
    cleanup();
    done->set_value();
  });
  // Teardown doesn't block the JavaScript thread, so the cleanup runs soon. The
  // timeout keeps a stuck JavaScript thread from also freezing this one.
  if (finished.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    std::fprintf(stderr, "MaplibreHost: cleanup timed out\n");
  }
}

}  // namespace mln::example
