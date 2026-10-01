// The library's own MapLibre run loop, for Android and the browser. Every thread
// that owns one runs it continuously and blocks on a condition variable that
// timers and submitted work wake. The loop calls nothing outside the C++
// standard library:
//
// - A browser pthread blocks here, so no loop depends on returning to the
//   browser event loop.
// - MapLibre's Android loop polls through ALooper_pollOnce(), which flushes the
//   thread's binder commands on every poll. libbinder tears down its thread
//   state when the process exits, so a thread that polled during exit crashed.
//
// Nothing on either platform watches a file descriptor, so addWatch() throws.

#include <cassert>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <mln/actor/scheduler.hpp>
#include <mln/util/run_loop.hpp>

#include "run_loop_wake.hpp"
#include "testing/sync_point.hpp"

namespace mln {
namespace util {

class RunLoop::Impl {
 public:
  RunLoop::Type type = RunLoop::Type::Default;
  platform::RunLoopWake wake;
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

// One pass for a caller that drives the loop itself. run() is the only driver
// this platform uses.
void RunLoop::runOnce() {
  MBGL_VERIFY_THREAD(tid);
  process();
  impl->wake.processRunnables();
}

void RunLoop::stop() {
  // invoke() publishes and wakes under the queue lock. The worker can destroy
  // this loop as soon as that lock is released, so touch no members afterwards.
  invoke([this] { impl->running = false; });
  mln::testing::hit(mln::testing::SyncPoint::RunLoopStopSubmitted);
}

void RunLoop::updateTime() {}

// Runs this loop until its own work is done, ignoring the tag. A runtime worker
// runs its own queues.
//
// One pass runs everything outstanding, so a pass that leaves work behind found
// it queued while the pass ran. That bound rests on runTask() unlisting
// whatever it finds; see async_task.cpp.
void RunLoop::waitForEmpty(
  [[maybe_unused]] const mln::util::SimpleIdentity tag
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
  throw std::runtime_error("RunLoop::addWatch is not supported");
}

void RunLoop::removeWatch(int) {}

}  // namespace util
}  // namespace mln
