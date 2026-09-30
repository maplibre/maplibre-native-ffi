// The per-case hang watchdog. A case that stalls past its budget is named on
// stdout and stderr and the process aborts, so an emulator, simulator, or
// browser run reports which case hung instead of only its own timeout.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>

#include "harness.h"

namespace {

constexpr auto case_budget = std::chrono::seconds{30};

struct Watchdog {
  std::mutex mutex;
  std::condition_variable condition;
  // Bumped on every arm and disarm, so the thread can tell whether the case
  // it timed is still the one running.
  std::uint64_t serial = 0;
  bool armed = false;
  const char* file = nullptr;
  const char* name = nullptr;
  std::chrono::steady_clock::time_point deadline;
  bool started = false;
};

auto watchdog() -> Watchdog& {
  // Leaked on purpose: the detached thread may still hold it at process exit.
  static auto* instance = new Watchdog{};
  return *instance;
}

std::atomic<const char*> waiting_on{nullptr};
// Set on the thread that runs the cases. Waits on library threads, such as a
// transform that blocks or a gate parked in a completion, leave the note alone
// so the report names what the case itself is waiting on.
thread_local bool runs_cases = false;

[[noreturn]] void report_hang(const char* file, const char* name) {
  const char* what = waiting_on.load();
  for (auto* stream : {stdout, stderr}) {
    std::fprintf(
      stream, "\nMLN_TEST_HANG %s:%s%s%s\n", file, name,
      what == nullptr ? "" : " waiting on ", what == nullptr ? "" : what
    );
    std::fflush(stream);
  }
  std::abort();
}

void run(Watchdog& state) {
  auto lock = std::unique_lock{state.mutex};
  for (;;) {
    state.condition.wait(lock, [&state] { return state.armed; });
    const auto serial = state.serial;
    const auto deadline = state.deadline;
    const auto still_running = [&state, serial] {
      return state.serial != serial;
    };
    if (!state.condition.wait_until(lock, deadline, still_running)) {
      report_hang(state.file, state.name);
    }
  }
}

}  // namespace

extern "C" auto mln_test_now_milliseconds(void) -> uint64_t {
  return static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()
    )
      .count()
  );
}

extern "C" void mln_test_watchdog_arm(const char* file, const char* name) {
  auto& state = watchdog();
  runs_cases = true;
  {
    const auto lock = std::scoped_lock{state.mutex};
    state.serial += 1;
    state.armed = true;
    state.file = file;
    state.name = name;
    state.deadline =
      std::chrono::steady_clock::now() + case_budget * mln_test_timeout_scale();
    if (!state.started) {
      state.started = true;
      std::thread{[&state] { run(state); }}.detach();
    }
  }
  waiting_on.store(nullptr);
  state.condition.notify_all();
}

extern "C" void mln_test_watchdog_disarm(void) {
  auto& state = watchdog();
  {
    const auto lock = std::scoped_lock{state.mutex};
    state.serial += 1;
    state.armed = false;
  }
  state.condition.notify_all();
}

extern "C" void mln_test_watchdog_note(const char* what) {
  if (!runs_cases) {
    return;
  }
  waiting_on.store(what);
}
