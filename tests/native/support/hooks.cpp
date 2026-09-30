// Forwarders to the library's test hooks and the browser run-loop probes. They
// reach below the public ABI, and move to the internal suite with the seams.

#include <array>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <thread>

#include "hooks.h"

#include "c_api/test_hooks.hpp"

#if defined(__EMSCRIPTEN__)
#include <mln/util/async_task.hpp>
#include <mln/util/run_loop.hpp>

#include <emscripten/heap.h>

#include "platform/emscripten/run_loop_wake.hpp"
#endif

extern "C" auto mln_test_completion_contract(void) -> const char* {
  return mln_test_hook_completion_contract();
}

extern "C" mln_status mln_test_render_session_blocking_operation_create(
  mln_render_session session, atomic_bool* entered, const atomic_bool* release,
  const mln_completion* completion
) {
  return mln_test_hook_enqueue_blocking_render_operation(
    session, entered, release, completion
  );
}

extern "C" mln_status mln_test_block_map_cleanup(
  mln_map map, atomic_bool* entered, const atomic_bool* release
) {
  return mln_test_hook_block_map_cleanup(map, entered, release);
}

extern "C" mln_status mln_test_pending_runtime_operation(
  mln_runtime runtime, atomic_bool* entered, void** out_operation,
  const mln_completion* completion
) {
  return mln_test_hook_enqueue_pending_runtime_operation(
    runtime, entered, out_operation, completion
  );
}

extern "C" void mln_test_complete_runtime_operation(void* operation) {
  mln_test_hook_complete_runtime_operation(operation);
}

#if defined(__EMSCRIPTEN__)
extern "C" bool mln_test_browser_stop_allows_immediate_destruction(void) {
  using mln::util::RunLoop;
  // Keep the poisoned pointer outside linear memory even if its size changes.
  if (emscripten_get_heap_size() >= 0x7f7f7f7f) return false;
  alignas(RunLoop) auto storage = std::array<std::byte, sizeof(RunLoop)>{};
  auto ready = std::promise<RunLoop*>{};
  auto worker = std::thread{[&] {
    auto* loop = std::construct_at(reinterpret_cast<RunLoop*>(storage.data()));
    loop->invoke([&] { ready.set_value(loop); });
    loop->run();
    std::destroy_at(loop);
    // Reuse retired storage before stop returns, exposing any late access.
    storage.fill(std::byte{0x7f});
  }};
  auto* loop = ready.get_future().get();
  mln::platform::emscripten::setStopSubmittedHook(
    [](void* context) { static_cast<std::thread*>(context)->join(); }, &worker
  );
  loop->stop();
  return !worker.joinable();
}

extern "C" bool mln_test_browser_async_task_runs_after_clock_advance(void) {
  using mln::platform::emscripten::RunLoopWake;
  struct AdvanceClock final : RunLoopWake::Runnable {
    auto dueTime() const -> mln::TimePoint override {
      const auto next = mln::Clock::now() + std::chrono::milliseconds{1};
      while (mln::Clock::now() < next) {
      }
      return mln::TimePoint::max();
    }
    void runTask() override {}
  };
  auto loop = mln::util::RunLoop{};
  auto* wake = static_cast<RunLoopWake*>(loop.getLoopHandle());
  auto advance = std::make_shared<AdvanceClock>();
  wake->addRunnable(advance);
  auto calls = 0;
  auto next_task = mln::util::AsyncTask{[&] { ++calls; }};
  auto task = mln::util::AsyncTask{[&] {
    ++calls;
    next_task.send();
  }};
  task.send();
  task.send();
  // Advance the clock after the scheduler captures its readiness cutoff and
  // before it examines the queued task. Immediate work stays ready, including
  // work sent during dispatch whose next delay must not overflow.
  const auto delay = wake->processRunnables();
  const auto ran_once = calls == 1 && delay == mln::Milliseconds::zero();
  loop.runOnce();
  loop.runOnce();
  wake->removeRunnable(advance);
  return ran_once && calls == 2;
}
#endif
