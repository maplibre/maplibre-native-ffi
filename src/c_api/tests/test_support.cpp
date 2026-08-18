#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "test_support.h"

#include "completion/completion.hpp"
#include "render/render_session_test_support.hpp"
#include "runtime/runtime.hpp"

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
    probe->condition.wait(lock, [probe]() { return probe->released; });
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
    probe->condition.wait(lock, [probe]() { return probe->completed; });
    return true;
  }
  return probe->condition.wait_for(
    lock, std::chrono::milliseconds{timeout_ms},
    [probe]() { return probe->completed; }
  );
}

extern "C" mln_status mln_test_completion_finish(
  mln_test_completion* completion
) {
  if (!mln_test_completion_wait(completion, -1)) {
    return MLN_STATUS_INVALID_STATE;
  }
  return mln_test_completion_status(completion);
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

extern "C" bool mln_test_completion_contract(void) {
  struct Probe {
    std::atomic_uint calls = 0;
    std::atomic_uint releases = 0;
    std::atomic_int phase = 0;
    std::atomic_int status = MLN_STATUS_INVALID_STATE;
  };
  const auto callback =
    +[](void* user_data, const mln_completion_result* result) -> void {
    auto& probe = *static_cast<Probe*>(user_data);
    auto expected = 0;
    if (!probe.phase.compare_exchange_strong(expected, 1)) {
      probe.phase.store(-1);
    }
    probe.status.store(result->status);
    probe.calls.fetch_add(1);
  };
  const auto release = +[](void* user_data) -> void {
    auto& probe = *static_cast<Probe*>(user_data);
    auto expected = 1;
    if (!probe.phase.compare_exchange_strong(expected, 2)) {
      probe.phase.store(-1);
    }
    probe.releases.fetch_add(1);
  };
  const auto descriptor = [&](Probe& probe) -> mln_completion {
    return {
      .size = sizeof(mln_completion),
      .callback = callback,
      .user_data = &probe,
      .release_user_data = release,
    };
  };

  auto inline_probe = Probe{};
  auto inline_completion =
    std::make_shared<mln::core::Completion>(descriptor(inline_probe));
  mln::core::complete_value(
    inline_completion, MLN_STATUS_OK, std::string{}, std::uint32_t{7}
  );
  if (inline_probe.calls.load() != 0) return false;
  inline_completion->accept();
  inline_completion->resolve([](const mln_completion&) {});
  if (
    inline_probe.calls.load() != 1 || inline_probe.releases.load() != 1 ||
    inline_probe.phase.load() != 2 ||
    inline_probe.status.load() != MLN_STATUS_OK
  )
    return false;

  auto rejected_probe = Probe{};
  {
    auto rejected =
      std::make_shared<mln::core::Completion>(descriptor(rejected_probe));
    rejected->reject();
  }
  if (rejected_probe.calls.load() != 0 || rejected_probe.releases.load() != 0)
    return false;

  auto abandoned_probe = Probe{};
  {
    auto abandoned =
      std::make_shared<mln::core::Completion>(descriptor(abandoned_probe));
    abandoned->accept();
  }
  if (
    abandoned_probe.calls.load() != 1 || abandoned_probe.releases.load() != 1 ||
    abandoned_probe.status.load() != MLN_STATUS_CANCELLED
  )
    return false;

  for (auto iteration = 0; iteration < 100; ++iteration) {
    auto race_probe = Probe{};
    auto completion =
      std::make_shared<mln::core::Completion>(descriptor(race_probe));
    auto accept = std::thread{[completion]() { completion->accept(); }};
    auto resolve = std::thread{[completion]() {
      mln::core::complete(completion, MLN_STATUS_OK);
    }};
    accept.join();
    resolve.join();
    if (
      race_probe.calls.load() != 1 || race_probe.releases.load() != 1 ||
      race_probe.phase.load() != 2
    )
      return false;
  }
  return true;
}

extern "C" mln_status mln_test_runtime_reserve_child(mln_runtime runtime) {
  auto live = mln::core::lease_runtime(runtime);
  if (live == nullptr) return MLN_STATUS_INVALID_ARGUMENT;
  return live->control.reserve_child() ? MLN_STATUS_OK
                                       : MLN_STATUS_INVALID_STATE;
}

extern "C" void mln_test_runtime_abandon_child(mln_runtime runtime) {
  auto live = mln::core::lease_runtime(runtime);
  if (live != nullptr) live->control.abandon_child_reservation();
}

extern "C" mln_status mln_test_render_session_blocking_operation_create(
  mln_render_session session, atomic_bool* entered, const atomic_bool* release,
  const mln_completion* completion
) {
  if (entered == nullptr || release == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return mln::core::enqueue_blocking_test_render_operation(
    session, entered, release, completion
  );
}
