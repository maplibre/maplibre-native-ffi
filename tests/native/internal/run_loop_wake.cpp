#include <condition_variable>
#include <mutex>
#include <thread>

#include "platform/run_loop/run_loop_wake.hpp"

#include "support/harness.h"
#include "unity.h"

namespace {

void racing_wakes_survive_repeated_waits() {
  auto wake = mln::platform::RunLoopWake{};
  auto mutex = std::mutex{};
  auto condition = std::condition_variable{};
  auto consumed = 0;
  constexpr auto rounds = 1000;
  auto worker = std::thread{[&] {
    for (auto i = 0; i < rounds; ++i) {
      wake.wait(mln::Milliseconds{-1});
      {
        const auto lock = std::lock_guard{mutex};
        ++consumed;
      }
      condition.notify_one();
    }
  }};
  for (auto i = 0; i < rounds; ++i) {
    wake.notify();
    auto lock = std::unique_lock{mutex};
    condition.wait(lock, [&] { return consumed > i; });
  }
  worker.join();
  TEST_ASSERT_EQUAL_INT(rounds, consumed);
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(racing_wakes_survive_repeated_waits); }
