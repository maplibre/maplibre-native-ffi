#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "test_support.h"

#include "c_api/test_hooks.hpp"

namespace {

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

// A completion the executor never settles is a defect in the code under test,
// so every wait in this file is bounded: the caller reports a failed wait as a
// named test failure rather than letting the suite run out its CTest timeout.
constexpr auto completion_wait_limit_milliseconds = std::int64_t{60000};
constexpr auto completion_wait_limit =
  std::chrono::milliseconds{completion_wait_limit_milliseconds};

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
}

void release_completion(void* user_data) noexcept {
  auto* probe = static_cast<CompletionState*>(user_data);
  if (probe == nullptr) return;
  {
    const auto lock = std::scoped_lock{probe->mutex};
    probe->released = true;
  }
  probe->condition.notify_all();
}

}  // namespace

extern "C" mln_test_completion mln_test_completion_default(
  const size_t value_size
) {
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

extern "C" mln_test_completion mln_test_completion_readback(void) {
  auto completion =
    mln_test_completion_default(sizeof(mln_texture_readback_result));
  state(&completion)->copy_readback = true;
  return completion;
}

extern "C" mln_test_completion mln_test_completion_buffer_view(void) {
  auto completion = mln_test_completion_default(sizeof(mln_buffer_view));
  state(&completion)->copy_buffer_view = true;
  return completion;
}

extern "C" void mln_test_completion_destroy(mln_test_completion* completion) {
  auto* probe = state(completion);
  if (probe == nullptr) return;
  {
    auto lock = std::unique_lock{probe->mutex};
    // Bounded so a completion the library never releases fails the test that
    // submitted it instead of hanging the suite. The probe is deliberately
    // leaked in that case: a late release would write through this pointer.
    if (!probe->condition.wait_for(lock, completion_wait_limit, [probe]() {
          return probe->released;
        })) {
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

extern "C" bool mln_test_completion_wait(
  mln_test_completion* completion, const int64_t timeout_ms
) {
  auto* probe = state(completion);
  if (probe == nullptr) return false;
  auto lock = std::unique_lock{probe->mutex};
  if (timeout_ms < 0) {
    return probe->condition.wait_for(lock, completion_wait_limit, [probe]() {
      return probe->completed;
    });
  }
  return probe->condition.wait_for(
    lock, std::chrono::milliseconds{timeout_ms},
    [probe]() { return probe->completed; }
  );
}

extern "C" mln_status mln_test_completion_finish(
  mln_test_completion* completion
) {
  if (!mln_test_completion_wait(
        completion, completion_wait_limit_milliseconds
      )) {
    return MLN_STATUS_INVALID_STATE;
  }
  return mln_test_completion_status(completion);
}

extern "C" mln_status mln_test_completion_settle(
  mln_test_completion* completion
) {
  const auto status = mln_test_completion_finish(completion);
  mln_test_completion_destroy(completion);
  return status;
}

extern "C" mln_status mln_test_completion_finish_value(
  mln_test_completion* completion, void* out_value, const size_t value_size
) {
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

extern "C" bool mln_test_completion_poll(mln_test_completion* completion) {
  auto* probe = state(completion);
  if (probe == nullptr) return false;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->completed;
}

extern "C" mln_status mln_test_completion_status(
  mln_test_completion* completion
) {
  auto* probe = state(completion);
  if (probe == nullptr) return MLN_STATUS_INVALID_ARGUMENT;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->completed ? probe->status : MLN_STATUS_INVALID_STATE;
}

extern "C" uint32_t mln_test_completion_disposition(
  mln_test_completion* completion
) {
  auto* probe = state(completion);
  if (probe == nullptr) return MLN_COMMAND_DISPOSITION_CANCELLED;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->disposition;
}

extern "C" uint64_t mln_test_completion_generation(
  mln_test_completion* completion
) {
  auto* probe = state(completion);
  if (probe == nullptr) return 0;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->generation;
}

extern "C" const char* mln_test_completion_diagnostic(
  mln_test_completion* completion
) {
  auto* probe = state(completion);
  if (probe == nullptr) return "invalid completion probe";
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->diagnostic.c_str();
}

extern "C" size_t mln_test_completion_value_count(
  mln_test_completion* completion
) {
  auto* probe = state(completion);
  if (probe == nullptr) return 0;
  const auto lock = std::scoped_lock{probe->mutex};
  return probe->value_count;
}

extern "C" bool mln_test_completion_copy_value(
  mln_test_completion* completion, void* out_value, const size_t value_size
) {
  auto* probe = state(completion);
  if (probe == nullptr || out_value == nullptr) return false;
  const auto lock = std::scoped_lock{probe->mutex};
  if (!probe->completed || probe->value.size() != value_size) return false;
  std::memcpy(out_value, probe->value.data(), value_size);
  return true;
}

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
