// The browser run loop, replacing MapLibre's libuv-based default. It serves two
// kinds of owner thread:
//
//   * A worker pthread can block, so run() parks on a condition variable.
//   * The browser main thread cannot block — Atomics.wait throws there — so a
//     host on it drives runOnce() from mln_runtime_pump().
//
// A host driving runOnce() waits outside this loop, so the delay run() would
// have parked for becomes a wake instead; see DeadlineWake below.
//
// FD watches have no browser equivalent, so addWatch() throws.

#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>

#include <emscripten/eventloop.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <mln/actor/scheduler.hpp>
#include <mln/util/chrono.hpp>
#include <mln/util/run_loop.hpp>

#include "run_loop_wake.hpp"

namespace mln {
namespace util {
namespace {

// Reports a runnable's future due time to whatever drives this loop. Everything
// else reports only work that is ready now, so a host driving the loop from
// mln_runtime_pump() would park with its wake flag clear until it happened to
// pump again — never, if it parks on a negative timeout.
//
// The timer is armed on the main thread, because the wake has to come from a
// thread other than the one it releases: a worker parked in a futex wait serves
// none of its own JavaScript.
class DeadlineWake : public std::enable_shared_from_this<DeadlineWake> {
 public:
  explicit DeadlineWake(platform::emscripten::RunLoopWake& wake_)
      : wake(&wake_) {}

  // Arms a wake for `delay`, from the owner thread. Zero asks for nothing:
  // nextDelay() rounds up, so zero means the earliest runnable is already due.
  //
  // A wake outstanding for an earlier deadline stands in for this one, which
  // keeps a host pumping at frame rate from arming a timer per frame. An
  // outstanding wake is never cancelled, so a superseded one costs a single
  // pump that finds nothing to do.
  void arm(Milliseconds delay) {
    if (delay <= Milliseconds::zero()) {
      return;
    }

    auto const deadline = Clock::now() + delay;
    {
      const std::lock_guard lock(mutex);
      if (!outstanding.empty() && *outstanding.begin() <= deadline) {
        return;
      }
      outstanding.insert(deadline);
    }

    // The delay travels rather than the deadline, because the two threads read
    // the monotonic clock through different browser contexts. The wake is late
    // by the proxy hop's latency.
    auto armed = std::make_unique<Armed>(Armed{
      .owner = shared_from_this(),
      .deadline = deadline,
      .delay_milliseconds =
        std::chrono::duration<double, std::milli>(delay).count(),
    });
    auto const proxied = emscripten_proxy_async(
      emscripten_proxy_get_system_queue(), emscripten_main_runtime_thread_id(),
      &DeadlineWake::armOnMainThread, armed.get()
    );
    if (proxied != 0) {
      // The main thread owns it now, and frees it when the timeout fires.
      static_cast<void>(armed.release());
      return;
    }

    // No timer will fire, so report readiness now: running the runnable early
    // beats waiting on a wake that never comes.
    fire(deadline);
  }

  // Cuts the arming loose from the run loop; the owner thread calls this before
  // the loop goes away.
  void detach() {
    const std::lock_guard lock(mutex);
    wake = nullptr;
  }

 private:
  struct Armed {
    std::shared_ptr<DeadlineWake> owner;
    TimePoint deadline;
    double delay_milliseconds;
  };

  static void armOnMainThread(void* argument) {
    auto* armed = static_cast<Armed*>(argument);
    emscripten_set_timeout(
      &DeadlineWake::fireOnMainThread, armed->delay_milliseconds, armed
    );
  }

  static void fireOnMainThread(void* argument) {
    auto const armed = std::unique_ptr<Armed>(static_cast<Armed*>(argument));
    armed->owner->fire(armed->deadline);
  }

  void fire(TimePoint deadline) {
    const std::lock_guard lock(mutex);
    auto const armed = outstanding.find(deadline);
    if (armed != outstanding.end()) {
      outstanding.erase(armed);
    }
    // Under the lock, which keeps the run loop's wake state alive across
    // detach(). notify() takes the host's locks and none of ours, so it stays
    // the innermost step. The main browser thread runs this and spins rather
    // than waiting, so every lock the owner thread takes must be brief.
    if (wake != nullptr) {
      wake->notify();
    }
  }

  std::mutex mutex;
  platform::emscripten::RunLoopWake* wake = nullptr;
  std::multiset<TimePoint> outstanding;
};

}  // namespace

class RunLoop::Impl {
 public:
  RunLoop::Type type = RunLoop::Type::Default;
  platform::emscripten::RunLoopWake wake;
  std::shared_ptr<DeadlineWake> deadline_wake =
    std::make_shared<DeadlineWake>(wake);
  bool running = false;
};

RunLoop* RunLoop::Get() {
  assert(static_cast<RunLoop*>(Scheduler::GetCurrent()));
  return static_cast<RunLoop*>(Scheduler::GetCurrent());
}

RunLoop::RunLoop(Type type) : impl(std::make_unique<Impl>()) {
  impl->type = type;
  // Timers and async tasks make work ready without going through push(), where
  // MapLibre reports queued work to the host, so they report it from here.
  // push() then reports twice, which costs a flag store.
  impl->wake.platform_wake = [this]() {
    if (platformCallback) {
      platformCallback();
    }
  };
  Scheduler::SetCurrent(this);
}

RunLoop::~RunLoop() {
  impl->deadline_wake->detach();
  Scheduler::SetCurrent(nullptr);
}

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
  // run() waits out the next delay itself; a host that drives runOnce() is not
  // here to wait, so the delay becomes a wake it can be released by.
  impl->deadline_wake->arm(impl->wake.processRunnables());
}

void RunLoop::stop() {
  invoke([&] { impl->running = false; });
  impl->wake.notify();
}

void RunLoop::updateTime() {}

// Runs this loop until its own work is done, ignoring the tag: an owner thread
// runs its own queues.
//
// One pass runs everything outstanding, so a pass that leaves work behind found
// it queued while the pass ran. That bound rests on runTask() unlisting
// whatever it finds; see async_task.cpp. The work runs directly rather than
// through runOnce(), whose browser timer costs a proxy hop per pass and has no
// waiter to release.
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
  throw std::runtime_error("RunLoop::addWatch is not supported on Emscripten");
}

void RunLoop::removeWatch(int) {}

}  // namespace util
}  // namespace mln
