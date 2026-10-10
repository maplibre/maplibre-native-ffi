#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "maplibre_native_c/completion.h"

namespace mln::core {

class Completion final {
 public:
  using Delivery = std::function<void(const mln_completion&)>;

  explicit Completion(const mln_completion& descriptor);
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
  mln_completion descriptor_{};
  State state_ = State::Pending;
  Delivery pending_;
};

auto validate_completion(const mln_completion* completion) -> mln_status;

// Delivers a failure from inside a Completion::resolve delivery. A success
// carries what its function's header declares, so it goes through
// CompletionValue or ValuelessCompletion instead; a status of MLN_STATUS_OK
// here is a native bug, which this reports as MLN_STATUS_NATIVE_ERROR.
auto deliver_failure(
  const mln_completion& descriptor, mln_status status,
  const std::string& diagnostic
) noexcept -> void;

// Resolves completion with a failure, as deliver_failure delivers one.
auto complete_failure(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic = {}
) noexcept -> void;

// Resolves a command's completion. Commands deliver no value, which the
// header schema enforces, so a committed command reports success here.
auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, std::uint64_t generation = 0, std::string diagnostic = {}
) noexcept -> void;

}  // namespace mln::core
