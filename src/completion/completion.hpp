#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c/completion.h"

namespace mln::core {

class Completion;

// Where one completion's terminal result goes: the host's callback, or a
// delivery queue. make_completion() makes the only sinks that deliver, so a
// submission that does not pass through it cannot construct a Completion.
class CompletionSink final {
 public:
  CompletionSink(const CompletionSink&) = delete;
  CompletionSink(CompletionSink&&) noexcept = default;
  auto operator=(const CompletionSink&) -> CompletionSink& = delete;
  auto operator=(CompletionSink&&) noexcept -> CompletionSink& = default;
  ~CompletionSink() = default;

  // Delivers one terminal result. A value that a queue cannot copy turns into
  // MLN_STATUS_NATIVE_ERROR, and its owned handles are disposed.
  auto deliver(
    mln_status status, std::uint32_t disposition, std::uint64_t generation,
    std::string_view diagnostic, const void* value, std::size_t value_count,
    std::uint32_t value_size, const delivery::CompletionCapture* capture
  ) const noexcept -> void;

 private:
  CompletionSink() = default;

  friend class Completion;
  friend auto make_completion(
    const mln_completion* descriptor, std::shared_ptr<Completion>& out
  ) -> mln_status;

  auto deliver_queued(
    const mln_completion_result& result,
    const delivery::CompletionCapture* capture
  ) const noexcept -> void;

  // Callback mode.
  mln_completion descriptor_{};
  // Queue mode.
  std::shared_ptr<delivery::DeliveryQueueState> queue_;
  void* user_data_ = nullptr;
  // Preallocated at acceptance, so a result that cannot be copied still
  // reaches the queue. Delivery consumes it at most once.
  mutable delivery::ItemPtr fallback_;
  // Set by the first deliver(), so a delivery that throws afterwards does not
  // report a second result.
  mutable bool delivered_ = false;
};

class Completion final {
 public:
  explicit Completion(CompletionSink sink) noexcept;
  Completion(const Completion&) = delete;
  Completion(Completion&&) = delete;
  auto operator=(const Completion&) -> Completion& = delete;
  auto operator=(Completion&&) -> Completion& = delete;
  ~Completion();

  auto accept() noexcept -> void;
  auto reject() noexcept -> void;
  auto revoke() noexcept -> void;
  // Installed before acceptance. Retirement covers both native work and the
  // delivery to the sink plus release, including cancellation on abandonment.
  auto set_retirement_callback(std::function<void()> callback) noexcept -> void;
  auto retire_work() noexcept -> void;
  template <typename Function>
  auto resolve(Function&& delivery) noexcept -> void {
    auto expected = State::Accepted;
    if (!state_.compare_exchange_strong(expected, State::Resolved)) return;
    deliver(std::forward<Function>(delivery));
  }

 private:
  enum class State : std::uint8_t { Created, Accepted, Rejected, Resolved };

  template <typename Function>
  auto deliver(Function&& delivery) noexcept -> void {
    try {
      std::invoke(std::forward<Function>(delivery), std::as_const(sink_));
    } catch (...) {
      // Completion delivery cannot reopen terminal native work, but a delivery
      // that threw before reaching the sink still owes the host its result.
      deliver_undelivered();
    }
    release();
  }
  auto deliver_undelivered() noexcept -> void;
  auto release() noexcept -> void;
  auto retire_part() noexcept -> void;

  CompletionSink sink_;
  std::atomic<State> state_ = State::Created;
  std::atomic_uint retirement_parts_ = 2;
  std::function<void()> retirement_callback_;
};

// Checks a completion descriptor's shape: exactly one of a callback and a
// delivery queue, and in queue mode, a live queue and no release_user_data.
auto validate_completion(const mln_completion* completion) -> mln_status;
// As validate_completion(), except that a command accepts a null completion,
// which discards its outcome.
auto validate_command_completion(const mln_completion* completion)
  -> mln_status;

// Validates a completion descriptor and constructs the Completion that
// delivers to its callback or queue.
auto make_completion(
  const mln_completion* descriptor, std::shared_ptr<Completion>& out
) -> mln_status;
// As make_completion(), except that a null descriptor succeeds with a null
// Completion, which complete_command() discards.
auto make_command_completion(
  const mln_completion* descriptor, std::shared_ptr<Completion>& out
) -> mln_status;

// Delivers a failure from inside a Completion::resolve delivery. A success
// carries what its function's header declares, so it goes through
// CompletionValue or ValuelessCompletion instead; a status of MLN_STATUS_OK
// here is a native bug, which this reports as MLN_STATUS_NATIVE_ERROR.
auto deliver_failure(
  const CompletionSink& sink, mln_status status, const std::string& diagnostic
) noexcept -> void;

auto complete_failure(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic = {}
) noexcept -> void;

auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, std::uint64_t generation = 0, std::string diagnostic = {}
) noexcept -> void;

}  // namespace mln::core
