// Browser runtime workers own continuously running loops. A worker pthread may
// block on its condition variable while timers and submitted work wake it.
//
// FD watches have no browser equivalent, so addWatch() throws.

#include <cassert>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/run_loop.hpp>

#include "run_loop_wake.hpp"

namespace mbgl {
namespace util {

class RunLoop::Impl {
 public:
  RunLoop::Type type = RunLoop::Type::Default;
  platform::emscripten::RunLoopWake wake;
  bool running = false;
};

RunLoop* RunLoop::Get() {
  assert(static_cast<RunLoop*>(Scheduler::GetCurrent()));
  return static_cast<RunLoop*>(Scheduler::GetCurrent());
}

RunLoop::RunLoop(Type type) : impl(std::make_unique<Impl>()) {
  impl->type = type;
  Scheduler::SetCurrent(this);
}

RunLoop::~RunLoop() { Scheduler::SetCurrent(nullptr); }

LOOP_HANDLE RunLoop::getLoopHandle() { return &Get()->impl->wake; }

void RunLoop::wake() { impl->wake.notify(); }

void RunLoop::run() {
  MBGL_VERIFY_THREAD(tid);
  impl->running = true;
  while (impl->running) {
    process();
    auto const timeout = impl->wake.processRunnables();

    std::size_t remaining = 0;
    {
      std::scoped_lock queue_lock(mutex);
      remaining = defaultQueue.size() + highPriorityQueue.size();
    }

    std::unique_lock wake_lock(impl->wake.wake_mutex);
    if (!impl->running) {
      break;
    }

    if (remaining == 0 && !impl->wake.notified) {
      auto const predicate = [&] {
        return impl->wake.notified || !impl->running;
      };
      if (timeout.count() < 0) {
        impl->wake.cv.wait(wake_lock, predicate);
      } else {
        impl->wake.cv.wait_for(wake_lock, timeout, predicate);
      }
    }
    impl->wake.notified = false;
  }
}

void RunLoop::runOnce() {
  MBGL_VERIFY_THREAD(tid);
  process();
  static_cast<void>(impl->wake.processRunnables());
}

void RunLoop::stop() {
  invoke([&] { impl->running = false; });
  impl->wake.notify();
}

void RunLoop::updateTime() {}

// Runs this loop until its own work is done, ignoring the tag. A runtime worker
// runs its own queues.
//
// One pass runs everything outstanding, so a pass that leaves work behind found
// it queued while the pass ran. That bound rests on runTask() unlisting
// whatever it finds; see async_task.cpp. The work runs directly rather than
// through runOnce(), whose browser timer costs a proxy hop per pass and has no
// waiter to release.
void RunLoop::waitForEmpty(
  [[maybe_unused]] const mbgl::util::SimpleIdentity tag
) {
  MBGL_VERIFY_THREAD(tid);
  while (true) {
    process();
    impl->wake.processRunnables();

    std::size_t remaining;
    {
      std::scoped_lock lock(mutex);
      remaining = defaultQueue.size() + highPriorityQueue.size();
    }

    if (remaining == 0 && impl->wake.emptyForWaitForEmpty()) {
      return;
    }
  }
}

void RunLoop::addWatch(int, Event, std::function<void(int, Event)>&&) {
  throw std::runtime_error("RunLoop::addWatch is not supported on Emscripten");
}

void RunLoop::removeWatch(int) {}

}  // namespace util
}  // namespace mbgl
