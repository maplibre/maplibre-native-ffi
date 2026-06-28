// Emscripten run loop without libuv. Default platform uses libuv for async wake
// and FD watches; libuv's Emscripten port is not wired in this build. Worker
// pthreads block on a condition variable; the main thread is driven by
// mln_runtime_run_once() from requestAnimationFrame.

#include <cassert>
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
  impl->wake.processRunnables();
}

void RunLoop::stop() {
  invoke([&] { impl->running = false; });
  impl->wake.notify();
}

void RunLoop::updateTime() {}

void RunLoop::waitForEmpty(
  [[maybe_unused]] const mbgl::util::SimpleIdentity tag
) {
  while (true) {
    std::size_t remaining;
    {
      std::scoped_lock lock(mutex);
      remaining = defaultQueue.size() + highPriorityQueue.size();
    }

    if (remaining == 0 && impl->wake.emptyForWaitForEmpty()) {
      return;
    }

    runOnce();
  }
}

void RunLoop::addWatch(int, Event, std::function<void(int, Event)>&&) {
  throw std::runtime_error("RunLoop::addWatch is not supported on Emscripten");
}

void RunLoop::removeWatch(int) {}

}  // namespace util
}  // namespace mbgl
