// The browser run loop. MapLibre's default one is built on libuv, whose event
// loop has no browser backing, so this replaces it rather than porting it.
//
// It has to serve two kinds of owner thread, because the C API lets a host run
// a runtime on whichever thread it created it on:
//
//   * A worker pthread can block, so run() parks on a condition variable the
//     way a native run loop does.
//   * The browser main thread cannot block -- Atomics.wait throws there -- so a
//     host on it drives runOnce() from mln_runtime_pump(), typically out of
//     requestAnimationFrame.
//
// A host driving runOnce() waits outside this loop, so the delay run() would
// have parked for becomes a wake instead; see DeadlineWake below.
//
// FD watches have no browser equivalent and nothing in this build asks for
// them, so addWatch() reports that rather than pretending.

#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/run_loop.hpp>

#include <emscripten/eventloop.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>

#include "run_loop_wake.hpp"

namespace mbgl {
namespace util {
namespace {

// Reports a runnable's due time to whatever drives this loop, covering the one
// window nothing else does. push(), addRunnable() and notify() all report work
// that is ready now; a runOnce() that leaves a timer pending has work that
// becomes ready later, and a host driving the loop from mln_runtime_pump()
// parks with its wake flag clear in the meantime. Without this it runs that
// timer whenever it next happens to pump, which for a host that parks on a
// negative timeout is never.
//
// The browser's timer is the clock, and it is armed on the main thread rather
// than the owner thread because the wake has to come from a thread other than
// the one it releases: a worker parked in a futex wait serves none of its own
// JavaScript until something wakes it.
class DeadlineWake : public std::enable_shared_from_this<DeadlineWake> {
 public:
  explicit DeadlineWake(platform::emscripten::RunLoopWake& wake_)
      : wake(&wake_) {}

  // Arms a wake for `delay`, from the owner thread. A delay of zero asks for
  // nothing: nextDelay() rounds up, so zero means the earliest runnable is
  // already due, and processRunnables() has just run everything that was.
  //
  // A wake already outstanding for an earlier deadline stands in for this one,
  // which is what keeps a host pumping at frame rate from arming a timer per
  // frame. A deadline that moves earlier arms a second wake and leaves the
  // first to fire: an outstanding wake is not cancelled, so the cost of one
  // that nothing needs any more is a single pump that finds nothing to do.
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
    // the monotonic clock through different browser contexts. It costs the
    // proxy hop's latency, which the wake is late by.
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

    // No timer will fire, so readiness is reported now instead: a host that
    // keeps pumping runs the runnable early, where one waiting on a wake that
    // never comes would not run it at all.
    fire(deadline);
  }

  // Cuts the arming loose from the run loop, which the owner thread does before
  // the loop goes away. Wakes still outstanding then only release their own
  // state.
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
    // Under the lock, which is what keeps the run loop's wake state alive
    // across detach(). notify() takes the host's locks and none of ours, so it
    // stays the innermost step.
    //
    // The main browser thread runs this, and Emscripten serves a lock it has to
    // wait for by spinning rather than by Atomics.wait. What it waits for is
    // bounded: the owner thread holds either lock for a flag store, a list
    // lookup, or a condition variable signal.
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
  // Timers and async tasks make work ready without going through push(), which
  // is where MapLibre reports queued work to the host, so they report it from
  // here instead. push() then reports twice, which costs a flag store: the
  // alternative is for a runnable to leave a host parked in
  // mln_runtime_pump() asleep with work waiting.
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

// Runs this loop until its own work is done.
//
// One pass runs everything outstanding: the queues drain, and so does every
// runnable that counts, because an async task is due the moment it is queued.
// A pass that leaves work behind found it queued while the pass ran, so each
// repeat runs what arrived rather than asking again for the same thing. That
// bound rests on runTask() unlisting whatever it finds; see async_task.cpp.
//
// It runs the work directly rather than through runOnce(), which arms a browser
// timer for the delay it leaves pending. A caller here is draining, so the wake
// that timer carries has nothing to release, and arming one costs a proxy hop
// to the main thread per pass.
//
// The reach stops at this loop. A caller arriving from ThreadedScheduler, where
// the tag selects one map's tasks across pool threads, gets this loop's own
// queues instead: an owner thread runs its own work. RenderSessionScheduler
// takes the same position; see render/render_session_common.hpp.
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
