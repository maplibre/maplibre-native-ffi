#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "delivery/delivery_queue.hpp"
#include "execution/control_state.hpp"
#include "maplibre_native_c/completion.h"

namespace mln::core {

class Completion;

// The generations a map command's completion reports: the snapshot that the
// command published, and the latest render update that snapshot reports.
struct CommandGenerations {
  std::uint64_t snapshot = 0;
  std::uint64_t render_update = 0;
};

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
    mln_status status, std::uint32_t disposition,
    CommandGenerations generations, std::string_view diagnostic,
    const void* value, std::size_t value_count, std::uint32_t value_size,
    const delivery::CompletionCapture* capture
  ) const noexcept -> void;

 private:
  CompletionSink() = default;

  friend class Completion;
  friend auto make_completion(
    const mln_completion* descriptor, std::shared_ptr<Completion>& out
  ) -> mln_status;
  friend auto make_completion(
    const mln_completion* descriptor, std::optional<Completion>& out
  ) -> mln_status;

  friend auto make_completion_sink(
    const mln_completion*, CompletionSink&, delivery::ItemPtr&
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
  Completion* owner_ = nullptr;
};

class Completion final {
 public:
  explicit Completion(CompletionSink sink, delivery::ItemPtr fallback) noexcept;
  Completion(const Completion&) = delete;
  Completion(Completion&&) = delete;
  auto operator=(const Completion&) -> Completion& = delete;
  auto operator=(Completion&&) -> Completion& = delete;
  ~Completion();

  auto accept() noexcept -> void;
  auto revoke() noexcept -> void;
  // Installed before acceptance. The completion holds its submission's lease
  // until the sink has its result and release_user_data has run, including
  // the cancellation an abandoned completion delivers.
  auto set_submission_lease(ControlLease lease) noexcept -> void;
  template <typename Function>
  auto resolve(Function&& delivery) noexcept -> void {
    auto expected = State::Accepted;
    if (!state_.compare_exchange_strong(expected, State::Resolved)) return;
    deliver(std::forward<Function>(delivery));
  }

 private:
  enum class State : std::uint8_t { Created, Accepted, Resolved };

  template <typename Function>
  auto deliver(Function&& delivery) noexcept -> void {
    try {
      std::invoke(std::forward<Function>(delivery), std::as_const(sink_));
    } catch (...) {
      // Completion delivery cannot reopen terminal native work, but a delivery
      // that threw before reaching the sink still owes the host its result.
      if (!delivered_) deliver_undelivered();
    }
    release();
  }
  auto deliver_undelivered() noexcept -> void;
  auto release() noexcept -> void;

  friend class CompletionSink;
  CompletionSink sink_;
  delivery::ItemPtr fallback_;
  bool delivered_ = false;
  std::atomic<State> state_ = State::Created;
  ControlLease submission_lease_;
};

// Validates a completion descriptor and constructs the Completion that
// delivers to its callback or queue.
auto make_completion(
  const mln_completion* descriptor, std::shared_ptr<Completion>& out
) -> mln_status;
auto make_completion(
  const mln_completion* descriptor, std::optional<Completion>& out
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
  Completion* completion, std::uint32_t disposition, mln_status status,
  CommandGenerations generations = {}, std::string diagnostic = {}
) noexcept -> void;
auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, CommandGenerations generations = {},
  std::string diagnostic = {}
) noexcept -> void;

}  // namespace mln::core
