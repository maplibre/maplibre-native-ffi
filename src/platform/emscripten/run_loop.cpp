// Emscripten run loop without libuv. Default platform uses libuv for async wake
// and FD watches; libuv's Emscripten port is not wired in this build. Worker
// pthreads block on a condition variable; the main thread is driven by
// mln_runtime_run_once() from requestAnimationFrame.

#include <cassert>
#include <chrono>
#include <cstdio>
#include <stdexcept>

#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/run_loop.hpp>

#include <emscripten.h>

#include "run_loop_wake.hpp"

namespace mbgl {
namespace util {

namespace {

std::atomic<platform::emscripten::RunLoopWake*> observedWake = nullptr;

auto elapsedMs(mbgl::TimePoint started) -> double {
  return std::chrono::duration<double, std::milli>(mbgl::Clock::now() - started)
    .count();
}

void traceRunLoop(
  const char* phase, double processMs,
  platform::emscripten::RunLoopWake::Stats stats
) {
  if (!platform::emscripten::run_loop_trace_enabled.load(
        std::memory_order_relaxed
      )) {
    return;
  }
  if (processMs < 30.0 && stats.elapsedMs < 30.0 && stats.readyCount < 100) {
    return;
  }
  std::fprintf(
    stderr,
    "browser run loop: %s process=%.3fms runnables=%.3fms ready=%zu "
    "total=%zu\n",
    phase, processMs, stats.elapsedMs, stats.readyCount, stats.runnableCount
  );
}

}  // namespace

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
  observedWake.store(&impl->wake, std::memory_order_relaxed);
}

RunLoop::~RunLoop() {
  auto* expected = &impl->wake;
  observedWake.compare_exchange_strong(expected, nullptr);
  Scheduler::SetCurrent(nullptr);
}

LOOP_HANDLE RunLoop::getLoopHandle() { return &Get()->impl->wake; }

void RunLoop::wake() { impl->wake.notify(); }

void RunLoop::run() {
  MBGL_VERIFY_THREAD(tid);
  impl->running = true;
  while (impl->running) {
    auto const processStart = mbgl::Clock::now();
    process();
    auto const processMs = elapsedMs(processStart);
    auto const timeout = impl->wake.processRunnables();
    traceRunLoop("run", processMs, impl->wake.stats());

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
  auto const processStart = mbgl::Clock::now();
  process();
  auto const processMs = elapsedMs(processStart);
  impl->wake.processRunnables();
  traceRunLoop("run_once", processMs, impl->wake.stats());
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

extern "C" {

EMSCRIPTEN_KEEPALIVE void mln_emscripten_run_loop_trace_set(int enabled) {
  mbgl::platform::emscripten::run_loop_trace_enabled.store(
    enabled != 0, std::memory_order_relaxed
  );
}

EMSCRIPTEN_KEEPALIVE auto mln_emscripten_run_loop_last_ready_count()
  -> std::size_t {
  auto* wake = mbgl::util::observedWake.load(std::memory_order_relaxed);
  return wake != nullptr ? wake->stats().readyCount : 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_emscripten_run_loop_last_runnable_count()
  -> std::size_t {
  auto* wake = mbgl::util::observedWake.load(std::memory_order_relaxed);
  return wake != nullptr ? wake->stats().runnableCount : 0;
}

EMSCRIPTEN_KEEPALIVE auto mln_emscripten_run_loop_last_runnables_ms()
  -> double {
  auto* wake = mbgl::util::observedWake.load(std::memory_order_relaxed);
  return wake != nullptr ? wake->stats().elapsedMs : 0.0;
}

}  // extern "C"
