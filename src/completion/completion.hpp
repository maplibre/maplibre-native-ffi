#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c/completion.h"

namespace mln::core {

class Completion;

// Where one completion's terminal result goes: the host's callback, or a
// delivery queue. accept_completion() makes the only sinks that deliver, so a
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
    const std::string& diagnostic, const void* value, std::size_t value_count,
    std::uint32_t value_size, const delivery::CompletionCapture* capture
  ) const noexcept -> void;

 private:
  CompletionSink() = default;

  friend class Completion;
  friend auto accept_completion(
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
};

class Completion final {
 public:
  using Delivery = std::function<void(const CompletionSink&)>;

  explicit Completion(CompletionSink sink) noexcept;
  Completion(const Completion&) = delete;
  Completion(Completion&&) = delete;
  auto operator=(const Completion&) -> Completion& = delete;
  auto operator=(Completion&&) -> Completion& = delete;
  ~Completion();

  auto accept() noexcept -> void;
  auto reject() noexcept -> void;
  auto resolve(Delivery delivery) noexcept -> void;

 private:
  enum class State : std::uint8_t { Pending, Accepted, Rejected, Resolved };

  auto deliver(Delivery delivery) noexcept -> void;
  auto release() noexcept -> void;

  std::mutex mutex_;
  CompletionSink sink_;
  State state_ = State::Pending;
  Delivery pending_;
};

// Checks a completion descriptor's shape and, in queue mode, that its queue is
// live. A submission that validates early calls this, then constructs its
// Completion through accept_completion().
auto validate_completion(const mln_completion* completion) -> mln_status;

// Validates a completion descriptor and constructs the Completion that
// delivers to its callback or queue.
auto accept_completion(
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
