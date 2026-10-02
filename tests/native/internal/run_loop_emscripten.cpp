// The browser run loop's scheduling: src/platform/run_loop/run_loop.cpp.

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <thread>

#include <mln/util/async_task.hpp>
#include <mln/util/run_loop.hpp>

#include <emscripten/heap.h>

#include "platform/run_loop/run_loop_wake.hpp"
#include "support/harness.h"
#include "testing/sync_point.hpp"
#include "unity.h"

namespace {

using mln::testing::SyncPoint;

// Immediate work stays ready when the clock advances after the scheduler
// captures its readiness cutoff and before it examines the queued task,
// including work sent during dispatch whose next delay must not overflow.
void queued_async_task_runs_when_clock_advances_during_dispatch() {
  using mln::platform::RunLoopWake;
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
  const auto delay = wake->processRunnables();
  const auto ran_once = calls == 1;
  const auto ready_now = delay == mln::Milliseconds::zero();
  loop.runOnce();
  loop.runOnce();
  wake->removeRunnable(advance);
  TEST_ASSERT_TRUE(ran_once);
  TEST_ASSERT_TRUE(ready_now);
  TEST_ASSERT_EQUAL_INT(2, calls);
}

struct StopProbe {
  std::thread* worker = nullptr;
  std::thread::id stopper;
  std::atomic_bool joined = false;
};

// stop() touches nothing of the loop once it has submitted its stop task, so
// the worker may destroy the loop and reuse its storage before stop() returns.
// The handler joins the worker at that point, which makes the reuse happen
// inside stop().
void stop_returns_after_its_worker_destroys_the_loop() {
  using mln::util::RunLoop;
  // Keep the poisoned pointer outside linear memory even if its size changes.
  TEST_ASSERT_LESS_THAN_size_t(0x7f7f7f7f, emscripten_get_heap_size());
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

  static auto probe = StopProbe{};
  probe.worker = &worker;
  probe.stopper = std::this_thread::get_id();
  probe.joined = false;
  // Runtimes from earlier cases may still stop loops on other threads, so the
  // handler acts only on this thread's stop.
  mln::testing::set_sync_point_handler(
    [](SyncPoint point, void*) noexcept {
      if (
        point == SyncPoint::RunLoopStopSubmitted &&
        std::this_thread::get_id() == probe.stopper && !probe.joined.load()
      ) {
        probe.worker->join();
        probe.joined = true;
      }
    },
    nullptr
  );
  loop->stop();
  mln::testing::set_sync_point_handler(nullptr, nullptr);
  const auto joined_inside_stop = probe.joined.load();
  if (worker.joinable()) worker.join();
  TEST_ASSERT_TRUE(joined_inside_stop);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(queued_async_task_runs_when_clock_advances_during_dispatch);
  RUN_TEST(stop_returns_after_its_worker_destroys_the_loop);
}
