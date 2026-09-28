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

auto invoke_completion(
  const mln_completion& descriptor, mln_status status,
  std::uint32_t disposition, std::uint64_t generation,
  const std::string& diagnostic, const void* value, std::size_t value_count
) noexcept -> void;

auto complete(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic = {}, const void* value = nullptr,
  std::size_t value_count = 0
) noexcept -> void;

auto complete_command(
  const std::shared_ptr<Completion>& completion, std::uint32_t disposition,
  mln_status status, std::uint64_t generation = 0, std::string diagnostic = {}
) noexcept -> void;

template <typename Value>
auto complete_value(
  const std::shared_ptr<Completion>& completion, mln_status status,
  std::string diagnostic, Value value
) noexcept -> void {
  completion->resolve([status, diagnostic = std::move(diagnostic),
                       value =
                         std::move(value)](const mln_completion& descriptor) {
    invoke_completion(
      descriptor, status, MLN_COMMAND_DISPOSITION_COMMITTED, 0, diagnostic,
      status == MLN_STATUS_OK ? &value : nullptr,
      status == MLN_STATUS_OK ? 1 : 0
    );
  });
}

}  // namespace mln::core
