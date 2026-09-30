#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "wait.h"

#include "harness.h"

namespace {

constexpr auto default_wait_milliseconds = std::uint64_t{10000};

// Re-check interval for state that changes without a pulse. Short enough that
// such a wait costs little, long enough that a waiter does not spin.
constexpr auto recheck_interval = std::chrono::milliseconds{2};

struct Pulse {
  std::mutex mutex;
  std::condition_variable condition;
  std::uint64_t generation = 0;
};

auto pulse() -> Pulse& {
  // Leaked on purpose: library threads may pulse during process exit.
  static auto* instance = new Pulse{};
  return *instance;
}

auto remaining(mln_test_deadline deadline) -> std::chrono::milliseconds {
  const auto now = mln_test_now_milliseconds();
  return std::chrono::milliseconds{
    deadline.expires_at_milliseconds > now
      ? deadline.expires_at_milliseconds - now
      : 0
  };
}

void pulse_wake(void* user_data) noexcept {
  (void)user_data;
  mln_test_pulse();
}

struct CompletionState {
  std::mutex mutex;
  std::condition_variable condition;
  bool completed = false;
  bool released = false;
  mln_status status = MLN_STATUS_INVALID_STATE;
  std::uint32_t disposition = MLN_COMMAND_DISPOSITION_COMMITTED;
  std::uint64_t generation = 0;
  std::string diagnostic;
  std::vector<std::byte> value;
  std::size_t value_count = 0;
  std::size_t value_size = 0;
  bool copy_readback = false;
  bool copy_buffer_view = false;
  std::vector<std::byte> nested;
};

auto state(mln_test_completion* completion) -> CompletionState* {
  return completion == nullptr
           ? nullptr
           : static_cast<CompletionState*>(completion->state);
}

void receive_completion(
  void* user_data, const mln_completion_result* result
) noexcept {
  auto* probe = static_cast<CompletionState*>(user_data);
  if (probe == nullptr || result == nullptr) return;
  {
    const auto lock = std::scoped_lock{probe->mutex};
    probe->status = result->status;
    probe->disposition = result->disposition;
    probe->generation = result->generation;
    probe->diagnostic.assign(
      static_cast<const char*>(result->diagnostic.data), result->diagnostic.size
    );
    probe->value_count = result->value_count;
    if (result->value != nullptr && probe->value_size != 0) {
      probe->value.resize(probe->value_size);
      std::memcpy(probe->value.data(), result->value, probe->value_size);
      if (probe->copy_readback) {
        auto* copied =
          reinterpret_cast<mln_texture_readback_result*>(probe->value.data());
        const auto* source = static_cast<const std::byte*>(copied->data.data);
        probe->nested.assign(source, source + copied->data.size);
        copied->data.data = probe->nested.data();
      } else if (probe->copy_buffer_view) {
        auto* copied = reinterpret_cast<mln_buffer_view*>(probe->value.data());
        const auto* source = static_cast<const std::byte*>(copied->data);
        if (copied->size != 0) {
          probe->nested.assign(source, source + copied->size);
          copied->data = probe->nested.data();
        }
      }
    }
    probe->completed = true;
  }
  probe->condition.notify_all();
  mln_test_pulse();
}

void release_completion(void* user_data) noexcept {
  auto* probe = static_cast<CompletionState*>(user_data);
  if (probe == nullptr) return;
  {
    const auto lock = std::scoped_lock{probe->mutex};
    probe->released = true;
  }
  probe->condition.notify_all();
  mln_test_pulse();
}

void discard_completion(
  void* user_data, const mln_completion_result* result
) noexcept {
  (void)user_data;
  (void)result;
}

void park_on_gate(
  void* user_data, const mln_completion_result* result
) noexcept {
  (void)result;
  mln_test_gate_park(static_cast<mln_test_gate*>(user_data));
}

void release_gate(void* user_data) noexcept { (void)user_data; }

auto flag_is_set(void* context) -> bool {
  return atomic_load(static_cast<const atomic_bool*>(context));
}

struct CountTarget {
  const atomic_int* counter;
  int target;
};

auto count_reached(void* context) -> bool {
  const auto* target = static_cast<const CountTarget*>(context);
  return atomic_load(target->counter) >= target->target;
}

}  // namespace

extern "C" auto mln_test_deadline_after(uint64_t milliseconds)
  -> mln_test_deadline {
  return mln_test_deadline{
    .expires_at_milliseconds =
      mln_test_now_milliseconds() + milliseconds * mln_test_timeout_scale(),
  };
}

extern "C" auto mln_test_deadline_default(void) -> mln_test_deadline {
  return mln_test_deadline_after(default_wait_milliseconds);
}

extern "C" auto mln_test_deadline_passed(mln_test_deadline deadline) -> bool {
  return mln_test_now_milliseconds() >= deadline.expires_at_milliseconds;
}

extern "C" void mln_test_pulse(void) {
  auto& state = pulse();
  {
    const auto lock = std::scoped_lock{state.mutex};
    state.generation += 1;
  }
  state.condition.notify_all();
}

extern "C" auto mln_test_pulse_wake(void) -> mln_wake {
  return mln_wake{
    .size = sizeof(mln_wake),
    .callback = pulse_wake,
    .user_data = nullptr,
    .release_user_data = nullptr,
  };
}

extern "C" auto mln_test_await(
  bool (*ready)(void* context), void* context, mln_test_deadline deadline,
  const char* what
) -> bool {
  mln_test_watchdog_note(what);
  auto& state = pulse();
  auto lock = std::unique_lock{state.mutex};
  for (;;) {
    const auto seen = state.generation;
    // The condition may call into the library, which may pulse, so it runs
    // without the pulse lock held.
    lock.unlock();
    const auto held = ready(context);
    lock.lock();
    if (held) {
      mln_test_watchdog_note(nullptr);
      return true;
    }
    const auto left = remaining(deadline);
    if (left.count() == 0) {
      mln_test_watchdog_note(nullptr);
      return false;
    }
    state.condition.wait_for(
      lock, std::min(left, std::chrono::milliseconds{recheck_interval}),
      [&state, seen] { return state.generation != seen; }
    );
  }
}

extern "C" void mln_test_flag_set(atomic_bool* flag) {
  atomic_store(flag, true);
  mln_test_pulse();
}

extern "C" auto mln_test_wait_for_flag_until(
  const atomic_bool* flag, mln_test_deadline deadline
) -> bool {
  return mln_test_await(
    flag_is_set, const_cast<atomic_bool*>(flag), deadline, "a flag"
  );
}

extern "C" auto mln_test_wait_for_flag(const atomic_bool* flag) -> bool {
  return mln_test_wait_for_flag_until(flag, mln_test_deadline_default());
}

extern "C" auto mln_test_wait_for_count(const atomic_int* counter, int target)
  -> bool {
  auto context = CountTarget{.counter = counter, .target = target};
  return mln_test_await(
    count_reached, &context, mln_test_deadline_default(), "a count"
  );
}

extern "C" void mln_test_gate_init(mln_test_gate* gate) {
  atomic_store(&gate->entered, false);
  atomic_store(&gate->released, false);
}

extern "C" void mln_test_gate_park(mln_test_gate* gate) {
  mln_test_flag_set(&gate->entered);
  (void)mln_test_wait_for_flag(&gate->released);
}

extern "C" auto mln_test_gate_wait_entered(mln_test_gate* gate) -> bool {
  return mln_test_await(
    flag_is_set, &gate->entered, mln_test_deadline_default(),
    "a thread to enter the gate"
  );
}

extern "C" void mln_test_gate_release(mln_test_gate* gate) {
  mln_test_flag_set(&gate->released);
}

extern "C" auto mln_test_gate_completion(mln_test_gate* gate)
  -> mln_completion {
  return mln_completion{
    .size = sizeof(mln_completion),
    .callback = park_on_gate,
    .user_data = gate,
    .release_user_data = release_gate,
  };
}

extern "C" auto mln_test_completion_default(const size_t value_size)
  -> mln_test_completion {
  auto* probe = new CompletionState{};
  probe->value_size = value_size;
  return mln_test_completion{
    .descriptor =
      mln_completion{
        .size = sizeof(mln_completion),
        .callback = receive_completion,
        .user_data = probe,
        .release_user_data = release_completion,
      },
    .state = probe,
  };
}

extern "C" auto mln_test_completion_readback(void) -> mln_test_completion {
  auto completion =
    mln_test_completion_default(sizeof(mln_texture_readback_result));
  state(&completion)->copy_readback = true;
  return completion;
}

extern "C" auto mln_test_completion_buffer_view(void) -> mln_test_completion {
  auto completion = mln_test_completion_default(sizeof(mln_buffer_view));
  state(&completion)->copy_buffer_view = true;
  return completion;
}

extern "C" auto mln_test_discard_completion(void) -> mln_completion {
  return mln_completion{
    .size = sizeof(mln_completion),
    .callback = discard_completion,
  };
}

extern "C" void mln_test_completion_destroy(mln_test_completion* completion) {
  auto* probe = state(completion);
  if (probe == nullptr) return;
  {
    auto lock = std::unique_lock{probe->mutex};
    // Bounded so a completion the library never releases fails the test that
    // submitted it instead of hanging the suite. The probe is deliberately
    // leaked in that case: a late release would write through this pointer.
    mln_test_watchdog_note("a completion's release");
    const auto released = probe->condition.wait_for(
      lock, remaining(mln_test_deadline_default()),
      [probe]() { return probe->released; }
    );
    mln_test_watchdog_note(nullptr);
    if (!released) {
      *completion = {};
      return;
    }
  }
  delete probe;
  *completion = {};
}

extern "C" void mln_test_completion_reject(mln_test_completion* completion) {
  if (
    completion == nullptr || completion->descriptor.release_user_data == nullptr
  ) {
    return;
  }
  completion->descriptor.release_user_data(completion->descriptor.user_data);
}

extern "C" auto mln_test_completion_wait(
  mln_test_completion* completion, const int64_t timeout_ms
) -> bool {
  auto* probe = state(completion);
  if (probe == nullptr) return false;
  const auto deadline =
    timeout_ms < 0
      ? mln_test_deadline_default()
      : mln_test_deadline_after(static_cast<std::uint64_t>(timeout_ms));
  auto lock = std::unique_lock{probe->mutex};
  mln_test_watchdog_note("a completion");
  const auto completed = probe->condition.wait_for(
    lock, remaining(deadline), [probe]() { return probe->completed; }
  );
  mln_test_watchdog_note(nullptr);
  return completed;
}

extern "C" auto mln_test_completion_finish(mln_test_completion* completion)
  -> mln_status {
  if (!mln_test_completion_wait(completion, -1)) {
    return MLN_STATUS_INVALID_STATE;
  }
  return mln_test_completion_status(completion);
}

extern "C" auto mln_test_completion_settle(mln_test_completion* completion)
  -> mln_status {
  const auto status = mln_test_completion_finish(completion);
  mln_test_completion_destroy(completion);
  return status;
}

extern "C" auto mln_test_completion_finish_value(
  mln_test_completion* completion, void* out_value, const size_t value_size
) -> mln_status {
  auto status = mln_test_completion_finish(completion);
  if (
    status == MLN_STATUS_OK &&
    !mln_test_completion_copy_value(completion, out_value, value_size)
  ) {
    status = MLN_STATUS_NATIVE_ERROR;
  }
  mln_test_completion_destroy(completion);
  return status;
}

extern "C" auto mln_test_completion_poll(mln_test_completion* completion)
  -> bool {
  auto* probe = state(completion);
  if (probe == nullptr) return false;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->completed;
}

extern "C" auto mln_test_completion_status(mln_test_completion* completion)
  -> mln_status {
  auto* probe = state(completion);
  if (probe == nullptr) return MLN_STATUS_INVALID_ARGUMENT;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->completed ? probe->status : MLN_STATUS_INVALID_STATE;
}

extern "C" auto mln_test_completion_disposition(mln_test_completion* completion)
  -> uint32_t {
  auto* probe = state(completion);
  if (probe == nullptr) return MLN_COMMAND_DISPOSITION_CANCELLED;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->disposition;
}

extern "C" auto mln_test_completion_generation(mln_test_completion* completion)
  -> uint64_t {
  auto* probe = state(completion);
  if (probe == nullptr) return 0;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->generation;
}

extern "C" auto mln_test_completion_diagnostic(mln_test_completion* completion)
  -> const char* {
  auto* probe = state(completion);
  if (probe == nullptr) return "invalid completion probe";
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->diagnostic.c_str();
}

extern "C" auto mln_test_completion_value_count(mln_test_completion* completion)
  -> size_t {
  auto* probe = state(completion);
  if (probe == nullptr) return 0;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->value_count;
}

extern "C" auto mln_test_completion_copy_value(
  mln_test_completion* completion, void* out_value, const size_t value_size
) -> bool {
  auto* probe = state(completion);
  if (probe == nullptr || out_value == nullptr) return false;
  const auto lock = std::scoped_lock{probe->mutex};
  if (!probe->completed || probe->value.size() != value_size) return false;
  std::memcpy(out_value, probe->value.data(), value_size);
  return true;
}
